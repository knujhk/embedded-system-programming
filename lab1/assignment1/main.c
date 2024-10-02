#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

#define SIZ 200
#define N_BUF 20    //int 8개 단위로 처리할거라서
#define N_WINDOWS 10
//#define NO_FREE

void* thread_function1(void* arg);


// read from external input (such as ADC, camera, connectivity, uart)
void receive_channel(int* buffer, int N) {
    for(int i=0;i<N;i++)
        buffer[i] = 1; // from ADC
}

int main(void)
{

    int i,j,k,a,w1,w2,r,c;
    int* A_col;
    int* B_row;
    int offset_i, offset_j;
    int res1;
    
    pthread_t a_thread1;
    pid_t pid;
    int flag;
    // 행렬 C는 전체를 동적으로 한번에 생성
    // 2-D Array using malloc
    //  아래처럼 하지 말고 중대조차도 malloc으 동적생성해보라~
    // int* C[SIZ]
    int** C = (int**)malloc(sizeof(int*) * SIZ);
    for(i=0; i<SIZ; i++) 
        C[i] = (int*)calloc(SIZ, sizeof(int));  //C의 모든 요소 0으로 초기화

    //res1 = pthread_create(&a_thread1, NULL ,thread_function1, NULL);

    // flag = fork();
    // if(flag == 0){
    //     sleep(1000);
    // }

    printf("matrix initialization wait:\n");
    scanf("%d", &a);

    //시간 찍기
    printf("matrix product:\n");
    clock_t before = clock();

    

    A_col = (int*)malloc(sizeof(int)*N_BUF);
    B_row = (int*)malloc(sizeof(int)*N_BUF);

    for(k = 0; k < SIZ; k++){
        for(w1 = 0; w1 < N_WINDOWS; w1++){
            offset_i = w1 * N_BUF;
            receive_channel(A_col,N_BUF);
            for(i = 0; i < N_BUF; i++){
                for(w2 = 0; w2 < N_WINDOWS; w2++){
                    offset_j = w2*N_BUF;
                    receive_channel(B_row,N_BUF);
                    for(j = 0; j < N_BUF; j++){
                        C[offset_i + i][offset_j + j] += A_col[i] * B_row[j];
                        //avx simd 어셈블리 명령을 사용하여 8개 int 한번에 계산하기
                        //변수 크기 alignment 필요
                    }
                }
            }  
        }
    }
    free(B_row);
    free(A_col);
    
    //시간 찍기
    clock_t after = clock();
    printf("matrix result:\n");
    printf("time : %lf\n",(double)(after-before) / CLOCKS_PER_SEC);

    FILE* fp = fopen("output.txt", "w");
    if(fp==NULL)
        printf("can't open output.txt\n");

    for(i=0;i<SIZ;i++){
        for(j=0;j<SIZ;j++){
            fprintf(fp, "%d ",C[i][j]);
        }            
        fprintf(fp,"\n");
    }

    printf("Press anything for exit:");
    scanf("%d", &a);
    return 0;
}

void* thread_function1(void* arg){
    int a,b,c;
    a = 2;
    b = 3;
    c = a+b;  
    sleep(10000);
}