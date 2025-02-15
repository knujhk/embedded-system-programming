#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "safe_malloc.h"

#define ITERATION 100000000
#define RAND 0    // 0~10
#define MAX_BLOCK_SIZE 10000
//#define SAFE //ifdef -> using safe_malloc 


V_PTR* vptr[10];
char* ptr[10];

extern int reuse_cnt;
extern int realloc_cnt;
extern int alloc_cnt;

int main(){
    initialize_vptr_arr();
    int garbage;

    struct timespec ts_start, ts_end;
    double elapsed;
    srand(time(NULL));//use rand() % n

    #ifndef SAFE
        printf("use malloc & free ..\n");
    #endif

    #ifdef SAFE
        printf("use safe_malloc & safe_free ..\n");    
    #endif

    clock_gettime(CLOCK_REALTIME,&ts_start);

    for(int i = 0; i < (ITERATION/10); i++){
        for(int j = 0; j < 10; j++){
            int r = rand() % 10;
            if(r >= 0 && r < RAND)
            {//랜덤 크기로 메모리 할당
                #ifndef SAFE
                garbage = r*sizeof(int) + MAX_BLOCK_SIZE/2;

                ptr[j] = malloc(rand() % (MAX_BLOCK_SIZE/2) + MAX_BLOCK_SIZE/2);
                free(ptr[j]);
                #endif
                #ifdef SAFE
                garbage = r*sizeof(int) + MAX_BLOCK_SIZE/2;

                vptr[j] = safe_malloc(rand() % (MAX_BLOCK_SIZE/2) + MAX_BLOCK_SIZE/2);
                safe_free(vptr[j]);
                #endif
            }
            else
            {//정해진 크기로 메모리 할당 r = 0,1,2.. RAND-1
                #ifndef SAFE
                garbage = rand() % (MAX_BLOCK_SIZE/2) + MAX_BLOCK_SIZE/2;

                ptr[j] = malloc(r*sizeof(int) + MAX_BLOCK_SIZE/2);
                free(ptr[j]);
                #endif
                #ifdef SAFE
                garbage = rand() % (MAX_BLOCK_SIZE/2) + MAX_BLOCK_SIZE/2;
                
                vptr[j] = safe_malloc(r*sizeof(int) + MAX_BLOCK_SIZE/2);
                safe_free(vptr[j]);
                #endif
            }
        }
    }

    clock_gettime(CLOCK_REALTIME,&ts_end);
    elapsed = ts_end.tv_sec-ts_start.tv_sec;
    elapsed += (double)(ts_end.tv_nsec-ts_start.tv_nsec)/1000000000.0;
    printf("%d iter, %d\% random alloc. time : %lf\n",ITERATION,RAND*10,elapsed);
    printf("max_block_size : %d\n",MAX_BLOCK_SIZE);
    printf("cnt :\n");
    printf("    alloc   = %d\n",alloc_cnt);
    printf("    realloc = %d\n",realloc_cnt);
    printf("    reuse   = %d\n",reuse_cnt);
    printf(" realloc : reuse = %.2lf : 1\n",realloc_cnt/(double)reuse_cnt);
}