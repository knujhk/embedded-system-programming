#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <immintrin.h>

#define SIZ 2000
#define N_BUF 100
#define N_WINDOWS 20
//#define NO_FREE

// read from external input (such as ADC, camera, connectivity, uart)
void receive_channel(int* buffer, int N) {
    for(int i=0;i<N;i++)
        buffer[i] = 1; // from ADC
}

int main(void)
{

    struct timespec ts_start, ts_end;
    time_t t_start, t_end;
    double elapsed;

    int i,j,k,a;
    int w1,w2;
    int *A_col, *A_row;
    int *B_col, *B_row;
    int offset_i, offset_j;
    
    int** C = (int**)malloc(sizeof(int*) * SIZ);
    for(i=0; i<SIZ; i++) 
        C[i] = (int*)calloc(SIZ, sizeof(int));  //C의 모든 요소 0으로 초기화

    printf("current process id: %d\n",getpid());

    printf("matrix initialization wait:\n");
    scanf("%d", &a);

    //시간 찍기
    printf("matrix product:\n");
    //clock_t before = clock();
    clock_gettime(CLOCK_REALTIME,&ts_start);

    
    __m128i vr = _mm_setzero_si128();

    //ijk - receive_channel 오버헤드 너무 큼 3배 느려짐
    // B_row = (int*)malloc(sizeof(int)*4);
    // receive_channel(B_row,4);
    // for(i = 0; i < SIZ; i++)
    // {
	// 	    A_row = (int*)malloc(sizeof(int)*SIZ);
	// 	    receive_channel(A_row,SIZ);

    //     for(j = 0; j < SIZ; j+=4)
    //     {
            
    //         vr = _mm_setzero_si128();
    //         for(k = 0; k < SIZ; k++)
    //         {
    //             //B_row = (int*)malloc(sizeof(int)*4);   //B_row k의 j~j+3
    //             //receive_channel(B_row,4);
    //             __m128i va = _mm_set1_epi32(A_row[k]); //A_row i의 k요소를 복사
    //             __m128i vb = _mm_loadu_si128((__m128i const*)(B_row)); 
    //             vr = _mm_add_epi32(vr, _mm_mullo_epi32(va, vb));
    //             //free(B_row);
    //         }
    //         _mm_storeu_si128((__m128i*)&(C[i][j]), vr);
            
    //     }
    //     free(A_row);
    // }
    // free(B_row);

    //ijk, i행, j열 벡터곱에 simd쓰고 hadd로 더하는 경우 - 20프로 향상
    // for(i = 0; i < SIZ; i++)
    // {
	// 	    A_row = (int*)malloc(sizeof(int)*SIZ);
	// 	    receive_channel(A_row,SIZ);
    //     for(j = 0; j < SIZ; j++)
    //     {
    //         B_col = (int*)malloc(sizeof(int)*SIZ);
    //         receive_channel(B_col,SIZ);  
    //         for(k = 0; k < SIZ; k+=4)
    //         {
    //             __m128i va = _mm_loadu_si128((__m128i const*)(A_row+k));
    //             __m128i vb = _mm_loadu_si128((__m128i const*)(B_col+k));
    //             vr = _mm_mullo_epi32(va, vb);
    //             //hadd로 4바이트 공간에 모아주는 계산 
    //             vr = _mm_hadd_epi32(vr,vr);
    //             vr = _mm_hadd_epi32(vr,vr);
    //             C[i][j] += _mm_extract_epi32(vr, 0);
    //             //C[i][j] += A_row[k] * B_col[k];
    //         }
    //         free(B_col);
    //     }
    //     free(A_row);
    // }

    //ikj
    // for(i = 0; i < SIZ; i++){
	// 	A_row = (int*)malloc(sizeof(int)*SIZ); //i row 
	//     receive_channel(A_row,SIZ);
    //     for(k = 0; k < SIZ; k++){
    //         B_row = (int*)malloc(sizeof(int)*SIZ); //k row
    //         receive_channel(B_row,SIZ);
    //         for(j = 0; j < SIZ; j+=4){
    //             __m128i va = _mm_set1_epi32(A_row[k]); 
    //             __m128i vb = _mm_loadu_si128((__m128i const*)(B_row+j));
    //             *(__m128i*)(&C[i][j]) = _mm_add_epi32(*(__m128i*)(&C[i][j]), _mm_mullo_epi32(va, vb));
    //             //C[i][j] += A_row[k] * B_row[j];
    //         }
    //         free(B_row);
    //     }
    //     free(A_row);
    // }

    //kij 여기다 적용하면 엄청 좋을 듯
    for(k = 0; k < SIZ; k++){
        A_col = (int*)malloc(sizeof(int)*SIZ);
        B_row = (int*)malloc(sizeof(int)*SIZ);
        receive_channel(A_col,SIZ);  //A의 k번째 열
        receive_channel(B_row,SIZ);  //B의 k번째 행

        for(i = 0; i < SIZ; i++){
            for(j = 0; j < SIZ; j+=4){
            __m128i va = _mm_set1_epi32(A_col[i]); 
            __m128i vb = _mm_loadu_si128((__m128i const*)(B_row+j));
            *(__m128i*)(&C[i][j]) = _mm_add_epi32(*(__m128i*)(&C[i][j]), _mm_mullo_epi32(va, vb));
            //C[i][j] += A_col[i] * B_row[j];
            }
        }
        free(A_col);
        free(B_row);
    }
    
    //시간 찍기
    printf("matrix result:\n");
    clock_gettime(CLOCK_REALTIME,&ts_end);
    elapsed = ts_end.tv_sec-ts_start.tv_sec;
    elapsed += (double)(ts_end.tv_nsec-ts_start.tv_nsec)/1000000000.0;
    printf("time : %lf\n",elapsed);
    //clock_t after = clock();
    //printf("time : %lf\n",(double)(after-before) / CLOCKS_PER_SEC);

    FILE* fp = fopen("output.txt", "w");
    if(fp==NULL)
        printf("can't open output.txt\n");

    for(i=0;i<SIZ;i++){
        for(j=0;j<SIZ;j++){
            fprintf(fp, "%d ",C[i][j]);
        }            
        fprintf(fp,"\n");
    }

    fclose(fp);
    for(i=0; i<SIZ; i++) 
        free(C[i]);
    free(C);

    printf("Press anything for exit:");
    scanf("%d", &a);
    return 0;
}