#include <stdio.h>

// malloc, free �Լ�
#include <stdlib.h>

// sqrt() �Լ�
#include <math.h>


int GetNumberOfData(char* fn)
{
	// ���� ���� �õ�
	FILE* fp = fopen(fn, "r");
	if (fp == NULL) {
		printf("���� '%s', ���� ����!! \n", fn);
		return -1;
	}

	char buf[100];
	int n_data = 0;
	// fgets() ���Ͽ��� ���� �о����, ���� �����͸� �����ٷ� �ѱ�
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		n_data += 1;
	}

	// ù���� header ����
	n_data = n_data - 1;

	// ���� �ݱ�
	fclose(fp);
	return n_data;
}

int ReadDataFile(char* fn, int  n_data, double** R, double** Q, double** E)
{
	// ���� ���� �õ�
	FILE* fp = fopen(fn, "r");
	if (fp == NULL) {
		// ������ �б� ���������� �̹� ���������� �׳� �ѹ� �� �غ�
		printf("���� '%s', ���� ����!! \n", fn);
		return -1;
	}

	// ������ �� �б� �غ�
	// �̸� ����� ������ �� ��ŭ 
	int index = 0;
	char buf[100];
	double* innerR = (double*)malloc(n_data * sizeof(double));
	double* innerQ = (double*)malloc(n_data * sizeof(double));
	double* innerE = (double*)malloc(n_data * sizeof(double));
	if (innerR == NULL || innerQ == NULL || innerE == NULL) {
		return -1;
	}

	// ù �� ������
	fgets(buf, sizeof(buf), fp);

	// ���� ������ �б� ����
	while (!feof(fp)) {
		char date[6];	// ��¥ �б�� (dummy)
		int ret = fscanf(fp, "%s %lf %lf %lf", date, &innerR[index], &innerQ[index], &innerE[index]);
		index += 1;
	}

	// ���� �ݱ�
	fclose(fp);

	// ���� ���Ǵ� �����Ϳ� �Ҵ�
	*R = innerR;
	*Q = innerQ;
	*E = innerE;
	return 0;
}

double ComputeDailyRunoff(
	double* h1,
	double* h2,
	double* h3,
	double R,
	double ETP,
	double a11,
	double a12,
	double a2,
	double a3,
	double z11,
	double z12,
	double z2,
	double b1,
	double b2
)
{
	double q11 = 0.0, q12 = 0.0, q2 = 0.0, q3 = 0.0;
	double p1 = 0.0, p2 = 0.0;
	double totalQ = 0.0;

	// 1�� ��ũ
	*h1 = *h1 + R - ETP;
	q11 = a11 * (*h1 - z11);
	q12 = a12 * (*h1 - z12);
	p1 = b1 * *h1;
	*h1 = *h1 - q11 - q12 - p1;

	// 2�� ��ũ
	*h2 = *h2 + p1;
	q2 = a2 * (*h2 - z2);
	p2 = b2 * *h2;
	*h2 = *h2 - q2 - p1;

	// 3�� ��ũ
	*h3 = *h3 + p2;
	q3 = a3 * *h3;
	*h3 = *h3 - q3 - p1;

	// ���� ���ⷮ
	totalQ = q11 + q12 + q2 + q3;
	return totalQ;
}

double RMSE(int n_data, double* observed, double* calculated)
{
	double sum = 0.0;
	for (int index = 0; index < n_data; ++index) {
		double error = observed[index] - calculated[index];
		double eSquared = error * error;

		sum += eSquared;
	}
	double ret = sqrt(sum / double(n_data));
	return ret;
}

int SaveResults(
	double a11,
	double a12,
	double a2,
	double a3,
	double z11,
	double z12,
	double z2,
	double b1,
	double b2,
	int n_data,
	double *calculatedQ,
	double *observedQ,
	double rmse
)
{
	FILE* fp = fopen("results.txt", "w");
	if (fp == NULL) {
		printf("\n���� ���� ����!\n");
		return -1;
	}

	// �Է°�
	fprintf(fp, "a11\ta12\ta2\ta3\tz11\tz12\tz2\tb1\tb2\n");
	fprintf(fp, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t\n", a11, a12, a2, a3, z11, z12, z2, b1, b2);

	// �����
	fprintf(fp, "\n\n");
	fprintf(fp, "calculated q\tobserved q\n");
	for (int index = 0; index < n_data; ++index) {
		fprintf(fp, "%lf\t%lf\n", calculatedQ[index], observedQ[index]);
	}
	fprintf(fp, "\n\n");
	fprintf(fp, "RMSE: %lf\n", rmse);

	fclose(fp);
	return 0;
}

int main(int argc, char* argv[])
{
	// ����ڰ� ������ �Է��ߴ��� Ȯ��
	if (argc != 2) {
		printf("������ڿ� ������ �Է��ϼ���.\n");
	}

	// �켱 ���Ͼȿ� ��� �����Ͱ� �����ϴ��� count
	int n_data = GetNumberOfData(argv[1]);
	if (n_data < 0) {
		// ���� �б� ����
		return -1;
	}
	printf("���� %s �ȿ� %d���� �����Ͱ� ������.\n", argv[1], n_data);

	// �Է� ������ ���� �б�
	double* R = NULL;
	double* Q = NULL;
	double* E = NULL;
	printf("�Էµ� '%s' ���� ������ �о� �ɴϴ�.\n", argv[1]);
	if (ReadDataFile(argv[1], n_data, &R, &Q, &E) < 0) {
		return -1;
	}

	// ���� q ���� ����
	double* calculatedQ = (double*)malloc(n_data * sizeof(double));

	// ����ġ �� rmse ��
	double threshold = 10.0;
	double rmse = 0.0;

	// ����ڿ��� �� �Է� �ޱ�
	double a11 = 0.0, a12 = 0.0, a2 = 0.0, a3 = 0.0;
	double z11 = 0.0, z12 = 0.0, z2 = 0.0;
	double b1 = 0.0, b2 = 0.0;

	//// ������ �Է� �ޱ� �������Ƿ� ������ �׳� �� ����
	//double a11 = 0.10, a12 = 0.16, a2 = 0.09, a3 = 0.007;
	//double z11 = 30.0, z12 = 15.0, z2 = 10.0;
	//double b1 = 0.1, b2 = 0.08;
	while (true) {
		printf("a11 �� �Է�: ");
		scanf("%lf", &a11);
		printf("a12 �� �Է�: ");
		scanf("%lf", &a12);
		printf("a2  �� �Է�: ");
		scanf("%lf", &a2);
		printf("a3  �� �Է�: ");
		scanf("%lf", &a3);
		printf("z11 �� �Է�: ");
		scanf("%lf", &z11);
		printf("z12 �� �Է�: ");
		scanf("%lf", &z12);
		printf("z2 �� �Է�: ");
		scanf("%lf", &z2);
		printf("b1 �� �Է�: ");
		scanf("%lf", &b1);
		printf("b2 �� �Է�: ");
		scanf("%lf", &b2);

		// �ʱⰪ
		double h1 = 0.0, h2 = 0.0, h3 = 30.0;
		printf("�ʱⰪ: h1: %lf, h2: %lf, h3: %lf\n", h1, h2, h3);

		// �Է°� Ȯ��
		printf("\n�Էµ� ��\n");
		printf("a11: %lf, a12: %lf, a2: %lf, a3: %lf\n", a11, a12, a2, a3);
		printf("z11: %lf, z12: %lf, z2: %lf\n", z11, z12, z2);
		printf("b1: %lf, b2: %lf\n", b1, b2);
		
		// ���
		for (int index = 0; index < n_data; ++index) {
			calculatedQ[index] = ComputeDailyRunoff(
				&h1, &h2, &h3,
				R[index], E[index],
				a11, a12, a2, a3, z11, z12, z2, b1, b2
			);
			//double error = Q[index] - calculatedQ[index];
			//printf("[%d]: h1: %lf, h2: %lf, h3: %lf, error: %lf\n", index, h1, h2, h3, error);
		}

		// RMSE ���
		rmse = RMSE(n_data, Q, calculatedQ);
		printf("\nRMSE: %lf\n", rmse);

		// rmse�� ���ذ����� ������ Ȯ��
		if (rmse < threshold) {
			break;
		}
		else {
			printf("RMSE �� %lf �� %lf ���� ũ�Ƿ� �ٽ� �Է� �޽��ϴ�", rmse, threshold);
		}
	}

	// ����� ���Ϸ� ����
	if (SaveResults(a11, a12, a2, a3, z11, z12, z2, b1, b2, n_data, calculatedQ, Q, rmse) < 0) {
		return -1;
	}

	// ������ �۾�
	// memory clean
	free(R);
	free(Q);
	free(E);
	free(calculatedQ);

	return 0;
}