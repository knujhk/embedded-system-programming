#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <immintrin.h>
#include <stdbool.h>


#define N_THREAD 8
#define STRIDE 4

#define SIZ 2000
#define N_BUF 100
#define N_WINDOWS 20
//#define NO_FREE

// read from external input (such as ADC, camera, connectivity, uart)
void receive_channel(int* buffer, int N) {
    for(int i=0;i<N;i++)
        buffer[i] = 1; // from ADC
}

void* thread_ijk_vectorProduct(void* arg);
void* thread_kij_vectorProduct(void* arg);
void* thread_ikj_vectorProduct(void* arg);

typedef struct thread_arg_ijk
{
    int i_start;
    int** C;
}thread_arg_ijk;

typedef struct thread_arg_ikj
{
    pid_t thread_id;
    int row_start;
    int row_end;
    int** B;
    int** C;
}thread_arg_ikj;

typedef struct thread_arg_kij
{
    pid_t thread_id;
    int k_start;
    int k_end;
    int** C;
}thread_arg_kij;

sem_t bin_sem;
int lock;

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
    
    int t;
    int res[N_THREAD];
    pthread_t thread[N_THREAD];
    void* thread_result;
    thread_arg_ijk ijk_arg[N_THREAD];
    thread_arg_ikj ikj_arg[N_THREAD];
    thread_arg_kij kij_arg[N_THREAD];

    sem_init(&bin_sem,0,1);

    int** B = (int**)malloc(sizeof(int*) * SIZ);
    for(i=0; i<SIZ; i++){
        B[i] = (int*)malloc(sizeof(int) * SIZ);
    }

    int** C = (int**)malloc(sizeof(int*) * SIZ);
    for(i=0; i<SIZ; i++){ 
        C[i] = (int*)calloc(SIZ, sizeof(int));  //C의 모든 요소 0으로 초기화
    }

    printf("current process id: %d\n",getpid());

    printf("matrix initialization wait:\n");
    scanf("%d", &a);

    //시간 찍기
    printf("matrix product:\n");
    //t_start = time(NULL);
    //clock_t before = clock();
    clock_gettime(CLOCK_REALTIME,&ts_start);

    //ijk
    // for(int i = 0; i < N_THREAD; i++){
    //     ijk_arg[i].i_start = i * STRIDE;
    //     ijk_arg[i].C = C;
    //     res[i] = pthread_create(&thread[i],NULL,thread_ijk_vectorProduct,&ijk_arg[i]);
    // }

    // for(int i = 0; i < N_THREAD; i++){
    //     pthread_join(thread[i], &thread_result);
    // }

    //ikj
    // for(int i = 0; i < SIZ; i++){
    //     receive_channel(B[i],SIZ);
    // }
    
    // printf("probe");
    // int width = SIZ / N_THREAD;
    // for(int i = 0; i < N_THREAD; i++){
    //     ikj_arg[i].row_start = i * width;
    //     ikj_arg[i].row_end = (i+1) * width;
    //     ikj_arg[i].B = B;
    //     ikj_arg[i].C = C;
    //     res[i] = pthread_create(&thread[i],NULL,thread_ikj_vectorProduct,&ikj_arg[i]);
    // }

    // for(int i = 0; i < N_THREAD; i++){
    //     pthread_join(thread[i], &thread_result);
    // }
    
    
    //kij
    //SIZ가 동적으로 변한다면? 멀티스레딩 적용 안 했을 땐 SIZ가 4의 배수이기만 하면 됨.
    //구조 변경 필요 - A의 k열 , B의 k행 가지고 siz*siz 매트릭스에 값 채워넣는 태스크 안에서 스레드 나누기
    //스레드 생성 소멸 오버헤드가 얼마나 영향을 미칠지도 생각해보자.
    int width = SIZ / N_THREAD;
    for(int i = 0; i < N_THREAD; i++){
        kij_arg[i].k_start = i * width; 
        kij_arg[i].k_end = (i+1) * width;
        kij_arg[i].C = C;
        kij_arg[i].thread_id = i;
        res[i] = pthread_create(&thread[i],NULL,thread_kij_vectorProduct,(void*)&kij_arg[i]);
    }

    for(int i = 0; i < N_THREAD; i++){
        pthread_join(thread[i],NULL);
    }

    

    //시간 찍기
    printf("matrix result:\n");
    clock_gettime(CLOCK_REALTIME,&ts_end);
    elapsed = ts_end.tv_sec-ts_start.tv_sec;
    elapsed += (double)(ts_end.tv_nsec-ts_start.tv_nsec)/1000000000.0;
    printf("time : %lf\n",elapsed);
    //t_end = time(NULL);
    //clock_t after = clock();
    //printf("time : %lf\n",(double)(after-before) / CLOCKS_PER_SEC);
    //printf("time : %ld\n",(t_end-t_start));

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

    sem_destroy(&bin_sem);

    printf("Press anything for exit:");
    scanf("%d", &a);
    return 0;
}

void* thread_ikj_vectorProduct(void* argument)
{
    thread_arg_ikj* arg = (thread_arg_ikj*)argument;
    int row_start = arg->row_start;
    int row_end = arg->row_end;
    int **B = arg->B;
    int **C = arg->C;
    pid_t thread_id = arg->thread_id;
    
    int* A_row;
    
    for(int i = row_start; i < row_end; i++){
        A_row = (int*)malloc(sizeof(int)*SIZ);
        receive_channel(A_row,SIZ);  

        for(int k = 0; k < SIZ; k++){
            int a = A_row[k];
            for(int j = 0; j < SIZ; j+=4){
            //synchronization을 하는 게 아니라 원자적 연산이 목적이라면 특수 명령어를 쓰는게 훨씬 빠르다.
            //애초에 각 스레드가 독립적으로 작업할 수 있도록 태스크를 나눴어야 함
            //mutual exclusion 비용을 간과했다.
            __m128i va = _mm_set1_epi32(a);
            __m128i vb = _mm_loadu_si128((__m128i const*)(B[i]+j));

            *(__m128i*)(&C[i][j]) = _mm_add_epi32(*(__m128i*)(&C[i][j]), _mm_mullo_epi32(va, vb));
            //C[i][j] += A_col[i] * B_row[j];
            //printf("thread %d working\n",thread_id);
            }
        }
        free(A_row);
    }
    
    return NULL;
}

void* thread_kij_vectorProduct(void* argument)
{
    thread_arg_kij* arg = (thread_arg_kij*)argument;
    int k_start = arg->k_start;
    int k_end = arg->k_end;
    int **C = arg->C;
    pid_t thread_id = arg->thread_id;

    int *A_col,*B_row;
    int prod;
    volatile bool check;
    
    for(int k = k_start; k < k_end; k++){
        A_col = (int*)malloc(sizeof(int)*SIZ);
        B_row = (int*)malloc(sizeof(int)*SIZ);
        receive_channel(A_col,SIZ);  //A의 k번째 열
        receive_channel(B_row,SIZ);  //B의 k번째 행

        // for(int i = 0; i < SIZ; i++){
        //     for(int j = 0; j < SIZ; j+=4){
        //     //synchronization을 하는 게 아니라 원자적 연산이 목적이라면 특수 명령어를 쓰는게 훨씬 빠르다.
        //     //애초에 각 스레드가 독립적으로 작업할 수 있도록 태스크를 나눴어야 함
        //     //mutual exclusion 비용을 간과했다.
        //     __m128i va = _mm_set1_epi32(A_col[i]);
        //     __m128i vb = _mm_loadu_si128((__m128i const*)(B_row+j));

        //     //spinrock
        //     while(check = __sync_bool_compare_and_swap(&lock,1,0)){

        //     }
        //     *(__m128i*)(&C[i][j]) = _mm_add_epi32(*(__m128i*)(&C[i][j]), _mm_mullo_epi32(va, vb));
        //     __sync_fetch_and_add(&lock,1);
        //     //C[i][j] += A_col[i] * B_row[j];
        //     //printf("thread %d working\n",thread_id);
        //     }
        // }
        //no simd
        for(int i = 0; i < SIZ; i++){
            for(int j = 0; j < SIZ; j++){
                C[i][j] += A_col[i] * B_row[j];
            //printf("thread %d working\n",thread_id);
            }
        }
        free(A_col);
        free(B_row);
    }
    
    return NULL;
}

void* thread_ijk_vectorProduct(void* argument)
{
    thread_arg_ijk* arg = (thread_arg_ijk*)argument;
    int* A_row;
    int* B_col;
    int** C = arg->C;
    int sum;

    int stride = STRIDE*N_THREAD;
    
    for(int i = arg->i_start; i < SIZ; i += stride)
    {
        for(int i_ = 0; i_ < STRIDE; i_++)
        {
            A_row = (int*)malloc(sizeof(int)*SIZ); // row i+i_ of A
            receive_channel(A_row,SIZ);
            for(int j = 0; j < SIZ; j++)
            {
                B_col = (int*)malloc(sizeof(int)*SIZ);
                receive_channel(B_col,SIZ);
                sum = 0;
                for(int k = 0; k < SIZ; k++)
                {
                    sum += A_row[k] * B_col[k];
                }
                //printf("C[%d][%d] = %d\n",i+i_,j,sum);
                C[i+i_][j] = sum;
                free(B_col);
            }
            free(A_row);
        }
    }

    return NULL;
}