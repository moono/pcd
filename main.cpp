#include <stdio.h>

// malloc, free 함수
#include <stdlib.h>

// sqrt() 함수
#include <math.h>


int GetNumberOfData(char* fn)
{
	// 파일 열기 시도
	FILE* fp = fopen(fn, "r");
	if (fp == NULL) {
		printf("파일 '%s', 열기 실패!! \n", fn);
		return -1;
	}

	char buf[100];
	int n_data = 0;
	// fgets() 파일에서 한줄 읽어오고, 파일 포인터를 다음줄로 넘김
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		n_data += 1;
	}

	// 첫줄은 header 줄임
	n_data = n_data - 1;

	// 파일 닫기
	fclose(fp);
	return n_data;
}

int ReadDataFile(char* fn, int  n_data, double** R, double** Q, double** E)
{
	// 파일 열기 시도
	FILE* fp = fopen(fn, "r");
	if (fp == NULL) {
		// 위에서 읽기 실패했으면 이미 나갔겠지만 그냥 한번 더 해봄
		printf("파일 '%s', 열기 실패!! \n", fn);
		return -1;
	}

	// 데이터 값 읽기 준비
	// 미리 계산한 데이터 양 만큼 
	int index = 0;
	char buf[100];
	double* innerR = (double*)malloc(n_data * sizeof(double));
	double* innerQ = (double*)malloc(n_data * sizeof(double));
	double* innerE = (double*)malloc(n_data * sizeof(double));
	if (innerR == NULL || innerQ == NULL || innerE == NULL) {
		return -1;
	}

	// 첫 줄 날리기
	fgets(buf, sizeof(buf), fp);

	// 실제 데이터 읽기 시작
	while (!feof(fp)) {
		char date[6];	// 날짜 읽기용 (dummy)
		int ret = fscanf(fp, "%s %lf %lf %lf", date, &innerR[index], &innerQ[index], &innerE[index]);
		index += 1;
	}

	// 파일 닫기
	fclose(fp);

	// 실제 사용되는 포인터에 할당
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

	// 1단 탱크
	*h1 = *h1 + R - ETP;
	q11 = a11 * (*h1 - z11);
	q12 = a12 * (*h1 - z12);
	p1 = b1 * *h1;
	*h1 = *h1 - q11 - q12 - p1;

	// 2단 탱크
	*h2 = *h2 + p1;
	q2 = a2 * (*h2 - z2);
	p2 = b2 * *h2;
	*h2 = *h2 - q2 - p1;

	// 3단 탱크
	*h3 = *h3 + p2;
	q3 = a3 * *h3;
	*h3 = *h3 - q3 - p1;

	// 계산된 유출량
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
		printf("\n파일 열기 실패!\n");
		return -1;
	}

	// 입력값
	fprintf(fp, "a11\ta12\ta2\ta3\tz11\tz12\tz2\tb1\tb2\n");
	fprintf(fp, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t\n", a11, a12, a2, a3, z11, z12, z2, b1, b2);

	// 결과값
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
	// 사용자가 파일을 입력했는지 확인
	if (argc != 2) {
		printf("명령인자에 파일을 입력하세요.\n");
	}

	// 우선 파일안에 몇개의 데이터가 존재하는지 count
	int n_data = GetNumberOfData(argv[1]);
	if (n_data < 0) {
		// 파일 읽기 실패
		return -1;
	}
	printf("파일 %s 안에 %d개의 데이터가 존재함.\n", argv[1], n_data);

	// 입력 데이터 파일 읽기
	double* R = NULL;
	double* Q = NULL;
	double* E = NULL;
	printf("입력된 '%s' 에서 파일을 읽어 옵니다.\n", argv[1]);
	if (ReadDataFile(argv[1], n_data, &R, &Q, &E) < 0) {
		return -1;
	}

	// 계산된 q 저장 공간
	double* calculatedQ = (double*)malloc(n_data * sizeof(double));

	// 기준치 및 rmse 값
	double threshold = 10.0;
	double rmse = 0.0;

	// 사용자에게 값 입력 받기
	double a11 = 0.0, a12 = 0.0, a2 = 0.0, a3 = 0.0;
	double z11 = 0.0, z12 = 0.0, z2 = 0.0;
	double b1 = 0.0, b2 = 0.0;

	//// 손으로 입력 받기 귀찮으므로 디버깅시 그냥 값 정의
	//double a11 = 0.10, a12 = 0.16, a2 = 0.09, a3 = 0.007;
	//double z11 = 30.0, z12 = 15.0, z2 = 10.0;
	//double b1 = 0.1, b2 = 0.08;
	while (true) {
		printf("a11 값 입력: ");
		scanf("%lf", &a11);
		printf("a12 값 입력: ");
		scanf("%lf", &a12);
		printf("a2  값 입력: ");
		scanf("%lf", &a2);
		printf("a3  값 입력: ");
		scanf("%lf", &a3);
		printf("z11 값 입력: ");
		scanf("%lf", &z11);
		printf("z12 값 입력: ");
		scanf("%lf", &z12);
		printf("z2 값 입력: ");
		scanf("%lf", &z2);
		printf("b1 값 입력: ");
		scanf("%lf", &b1);
		printf("b2 값 입력: ");
		scanf("%lf", &b2);

		// 초기값
		double h1 = 0.0, h2 = 0.0, h3 = 30.0;
		printf("초기값: h1: %lf, h2: %lf, h3: %lf\n", h1, h2, h3);

		// 입력값 확인
		printf("\n입력된 값\n");
		printf("a11: %lf, a12: %lf, a2: %lf, a3: %lf\n", a11, a12, a2, a3);
		printf("z11: %lf, z12: %lf, z2: %lf\n", z11, z12, z2);
		printf("b1: %lf, b2: %lf\n", b1, b2);
		
		// 계산
		for (int index = 0; index < n_data; ++index) {
			calculatedQ[index] = ComputeDailyRunoff(
				&h1, &h2, &h3,
				R[index], E[index],
				a11, a12, a2, a3, z11, z12, z2, b1, b2
			);
			//double error = Q[index] - calculatedQ[index];
			//printf("[%d]: h1: %lf, h2: %lf, h3: %lf, error: %lf\n", index, h1, h2, h3, error);
		}

		// RMSE 계산
		rmse = RMSE(n_data, Q, calculatedQ);
		printf("\nRMSE: %lf\n", rmse);

		// rmse가 기준값보다 작은지 확인
		if (rmse < threshold) {
			break;
		}
		else {
			printf("RMSE 값 %lf 가 %lf 보다 크므로 다시 입력 받습니다", rmse, threshold);
		}
	}

	// 결과값 파일로 저장
	if (SaveResults(a11, a12, a2, a3, z11, z12, z2, b1, b2, n_data, calculatedQ, Q, rmse) < 0) {
		return -1;
	}

	// 마무리 작업
	// memory clean
	free(R);
	free(Q);
	free(E);
	free(calculatedQ);

	return 0;
}