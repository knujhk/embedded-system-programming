#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZ 400
#define N_BUF 40
#define N_WINDOWS 10
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
    //clock_t before = clock();
    clock_gettime(CLOCK_REALTIME,&ts_start);


    //ijk loop
    // for(i=0;i<SIZ;i++){
    //     for(j=0;j<SIZ;j++)
    //     {
    //         for(int w = 0; w < N_WINDOWS; w++)
    //         {
    //             A_row = (int*)malloc(sizeof(int)*N_BUF); // A의 i행의 일부
    //             receive_channel(A_row,N_BUF);
    //             B_col = (int*)malloc(sizeof(int)*N_BUF); // B의 j열의 일부
    //             receive_channel(B_col,N_BUF);
    //             for(int t = 0; t < N_BUF; t+=4)
    //             {
    //                 C[i][j] += A_row[t] * B_col[t];
    //                 C[i][j] += A_row[t+1] * B_col[t+1];
    //                 C[i][j] += A_row[t+2] * B_col[t+2];
    //                 C[i][j] += A_row[t+3] * B_col[t+3];
    //                 // C[i][j] += A_row[t+4] * B_col[t+4];
    //                 // C[i][j] += A_row[t+5] * B_col[t+5];
    //                 // C[i][j] += A_row[t+6] * B_col[t+6];
    //                 // C[i][j] += A_row[t+7] * B_col[t+7];
    //                 // C[i][j] += A_row[t+8] * B_col[t+8];
    //                 // C[i][j] += A_row[t+9] * B_col[t+9];
    //             }
    //             free(A_row);
    //             free(B_col);
    //         }  
    //     }
    // }
           
    
    //ikj
    // for(i = 0; i < SIZ; i++)
    // {// i_row of A
    //     for(int w1 = 0; w1 < N_WINDOWS; w1++)
    //     {
    //         int k_offset = w1*N_BUF;
    //         A_row = (int*)malloc(sizeof(int)*N_BUF); //A_row_i[k_offset] ~ A_row_i[k_offset+N_BUF-1]
    //         receive_channel(A_row,N_BUF);

    //         for(int k = 0; k < N_BUF; k++)
    //         {
    //             for(int w2 = 0; w2 < N_WINDOWS; w2++)
    //             {
    //                 int j_offset = w2*N_BUF;
    //                 B_row = (int*)malloc(sizeof(int)*N_BUF); //B_row_K[j_offset] ~ A_row_K[j_offset+N_BUF-1]
    //                 receive_channel(B_row,N_BUF);

    //                 for(int j = 0; j < N_BUF; j+=1){
    //                     C[i][j_offset+j] += A_row[k] *  B_row[j];
    //                     // C[i][j_offset+j+1] += A_row[k] *  B_row[j+1];
    //                     // C[i][j_offset+j+2] += A_row[k] *  B_row[j+2];
    //                     // C[i][j_offset+j+3] += A_row[k] *  B_row[j+3];
    //                     // C[i][j_offset+j+4] += A_row[k] *  B_row[j+4];
    //                     // C[i][j_offset+j+5] += A_row[k] *  B_row[j+5];
    //                     // C[i][j_offset+j+6] += A_row[k] *  B_row[j+6];
    //                     // C[i][j_offset+j+7] += A_row[k] *  B_row[j+7];
    //                     // C[i][j_offset+j+8] += A_row[k] *  B_row[j+8];
    //                     // C[i][j_offset+j+9] += A_row[k] *  B_row[j+9];
    //                 }
    //                 free(B_row);
    //             }
    //         }
    //         free(A_row);
    //     }    
    // }

    // //kij loop
    int i_;

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
                    i_ = offset_i + i;
                    for(j = 0; j < N_BUF; j+=1){
                        C[i_][offset_j + j] += A_col[i] * B_row[j];
                        // C[offset_i + i][offset_j + j] += A_col[i] * B_row[j];
                        // C[offset_i + i][offset_j + j+1] += A_col[i] * B_row[j+1];
                        // C[offset_i + i][offset_j + j+2] += A_col[i] * B_row[j+2];
                        // C[offset_i + i][offset_j + j+3] += A_col[i] * B_row[j+3];
                        // C[offset_i + i][offset_j + j+4] += A_col[i] * B_row[j+4];
                        // C[offset_i + i][offset_j + j+5] += A_col[i] * B_row[j+5];
                        // C[offset_i + i][offset_j + j+6] += A_col[i] * B_row[j+6];
                        // C[offset_i + i][offset_j + j+7] += A_col[i] * B_row[j+7];
                        // C[offset_i + i][offset_j + j+8] += A_col[i] * B_row[j+8];
                        // C[offset_i + i][offset_j + j+9] += A_col[i] * B_row[j+9];
                    }
                    free(B_row);
                }
            }
            free(A_col);
        }
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