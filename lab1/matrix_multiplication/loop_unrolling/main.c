#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

    int i,j,k,a;
    int w1,w2;
    int* A_col, *A_row;
    int* B_col, *B_row;
    int offset_i, offset_j;
    
    int** C = (int**)malloc(sizeof(int*) * SIZ);
    for(i=0; i<SIZ; i++) 
        C[i] = (int*)calloc(SIZ, sizeof(int));  //C의 모든 요소 0으로 초기화

    printf("matrix initialization wait:\n");
    scanf("%d", &a);

    //시간 찍기
    printf("matrix product:\n");
    clock_t before = clock();

    //ijk loop 아직 미구현
    // for(i=0;i<SIZ;i++){
    //     for(j=0;j<SIZ;j++){
    //         // in this case, we assume only allocating on-demand, and free immediatly
    //         // sliding buffer for A by receiving from external sources
    //         for(k=0; k < N_WINDOWS; k++){
    //             //이 안에서 buf사이즈만큼 슬라이딩하면서 처리
    //             A_row = (int*)malloc(sizeof(int)*N_BUF);
    //             receive_channel(A_row,N_BUF);
    //              // sliding buffer for B by receiving from external sources
    //             B_col = (int*)malloc(sizeof(int)*N_BUF);
    //             receive_channel(B_col,N_BUF);
    //             for(int t = 0; t < N_BUF; t++){
    //                 C[i][j] += A_row[t] * B_col[t];
    //             }
    //             free(A_row);
    //             free(B_col);    
    //         }
    //     }            
    // }
    //ikj loop

    //kij loop
    for(k = 0; k < SIZ; k++){
        for(w1 = 0; w1 < N_WINDOWS; w1++){
            offset_i = w1 * N_BUF;
            A_col = (int*)malloc(sizeof(int)*N_BUF);
            receive_channel(A_col,N_BUF);//A의 K행
            for(i = 0; i < N_BUF; i++){
                for(w2 = 0; w2 < N_WINDOWS; w2++){
                    offset_j = w2*N_BUF;
                    B_row = (int*)malloc(sizeof(int)*N_BUF);
                    receive_channel(B_row,N_BUF);
                    for(j = 0; j < N_BUF; j++){
                        C[offset_i + i][offset_j + j] += A_col[i] * B_row[j];
                    }
                    // for(j = 0; j < N_BUF; j += 5){
                    //     C[offset_i + i][offset_j + j] += A_col[i] * B_row[j];
                    //     C[offset_i + i][offset_j + j + 1] += A_col[i] * B_row[j + 1];
                    //     C[offset_i + i][offset_j + j + 2] += A_col[i] * B_row[j + 2];
                    //     C[offset_i + i][offset_j + j + 3] += A_col[i] * B_row[j + 3];
                    //     C[offset_i + i][offset_j + j + 4] += A_col[i] * B_row[j + 4];
                    //     // C[offset_i + i][offset_j + j + 4] += A_col[i] * B_row[j + 5];
                    //     // C[offset_i + i][offset_j + j + 4] += A_col[i] * B_row[j + 6];
                    //     // C[offset_i + i][offset_j + j + 4] += A_col[i] * B_row[j + 7];
                    //     // C[offset_i + i][offset_j + j + 4] += A_col[i] * B_row[j + 8];
                    //     // C[offset_i + i][offset_j + j + 4] += A_col[i] * B_row[j + 9];
                    // }
                    free(B_row);
                }
            }
            free(A_col);
        }
    }
    
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

    fclose(fp);
    for(i=0; i<SIZ; i++) 
        free(C[i]);
    free(C);

    printf("Press anything for exit:");
    scanf("%d", &a);
    return 0;
}