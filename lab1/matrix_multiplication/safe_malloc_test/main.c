#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "safe_malloc.h"

#define SIZ 500
#define N_BUF 20    //int 8개 단위로 처리할거라서
#define N_WINDOWS 25
//#define NO_FREE

extern int reuse;
extern int alloc;
extern int realloc_cnt;
extern int real_free_cnt;
extern int virt_free_cnt;

// read from external input (such as ADC, camera, connectivity, uart)
void receive_channel(int* buffer, int N) {
    for(int i=0;i<N;i++)
        buffer[i] = 1; // from ADC
}

int main(void)
{
    initialize_vptr_arr();

    int i,j,k,a,w1,w2;
    int* A_col;
    int* B_row;
    int offset_i, offset_j;

    V_PTR* vptr1_a_col;
    int* ptr1_a_col;

    V_PTR* vptr2_b_row;
    int* ptr2_b_row;
    
    int** C = (int**)malloc(sizeof(int*) * SIZ);
    for(i=0; i<SIZ; i++) 
        C[i] = (int*)calloc(SIZ, sizeof(int));  //C의 모든 요소 0으로 초기화

    printf("matrix initialization wait:\n");
    scanf("%d", &a);

    //시간 찍기
    printf("matrix product:\n");
    clock_t before = clock();


    for(k = 0; k < SIZ; k++){
        for(w1 = 0; w1 < N_WINDOWS; w1++){
            offset_i = w1 * N_BUF;

            vptr1_a_col = safe_malloc(sizeof(int)*N_BUF);
            ptr1_a_col = vptr1_a_col->addr;
            
            receive_channel(ptr1_a_col,N_BUF);
            for(i = 0; i < N_BUF; i++){
                for(w2 = 0; w2 < N_WINDOWS; w2++){
                    offset_j = w2*N_BUF;

                    vptr2_b_row = safe_malloc(sizeof(int)*N_BUF);
                    ptr2_b_row = vptr2_b_row->addr;
                    
                    receive_channel(ptr2_b_row,N_BUF);
                    for(j = 0; j < N_BUF; j++){
                        C[offset_i + i][offset_j + j] += ptr1_a_col[i] * ptr2_b_row[j];
                        //avx simd 어셈블리 명령을 사용하여 8개 int 한번에 계산하기
                        //변수 크기 alignment 필요
                    }
                    safe_free(vptr2_b_row);
                }
            }
            safe_free(vptr1_a_col);  
        }
    }
    safe_free(vptr2_b_row);
    safe_free(vptr1_a_col);

    
    //시간 찍기
    clock_t after = clock();
    printf("matrix result:\n");
    printf("time : %lf\n",(double)(after-before) / CLOCKS_PER_SEC);

    //디버깅용
    printf("reuse %d, realloc %d, alloc %d, virt_free %d, real_free %d\n",reuse,realloc_cnt,alloc,virt_free_cnt,real_free_cnt);

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