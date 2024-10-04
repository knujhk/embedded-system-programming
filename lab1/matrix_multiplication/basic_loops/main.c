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
    int *A_col, *A_row;
    int *B_col, *B_row;
    int offset_i, offset_j;
    
    int** C = (int**)malloc(sizeof(int*) * SIZ);
    for(i=0; i<SIZ; i++) 
        C[i] = (int*)calloc(SIZ, sizeof(int));  //C의 모든 요소 0으로 초기화

    printf("matrix initialization wait:\n");
    scanf("%d", &a);

    //시간 찍기
    printf("matrix product:\n");
    clock_t before = clock();

    //ijk
    for(i = 0; i < SIZ; i++){
		    A_row = (int*)malloc(sizeof(int)*SIZ);
		    receive_channel(A_row,SIZ);
        for(j = 0; j < SIZ; j++){
            B_col = (int*)malloc(sizeof(int)*SIZ);
            receive_channel(B_col,SIZ);  
            for(k = 0; k < SIZ; k++){
                C[i][j] += A_row[k] * B_col[k];
            }
            free(B_col);
        }
        free(A_row);
    }

    //ikj
    for(i = 0; i < SIZ; i++){
		A_row = (int*)malloc(sizeof(int)*SIZ); //i row 
	    receive_channel(A_row,SIZ);
        for(k = 0; k < SIZ; k++){
            B_row = (int*)malloc(sizeof(int)*SIZ); //k row
            receive_channel(B_row,SIZ);
            for(j = 0; j < SIZ; j++){
                C[i][j] += A_row[k] * B_row[j];
            }
            free(B_row);
        }
        free(A_row);
    }

    //kij
    for(k = 0; k < SIZ; k++){
        A_col = (int*)malloc(sizeof(int)*SIZ);
        B_row = (int*)malloc(sizeof(int)*SIZ);
        receive_channel(A_col,SIZ);  //A의 k번째 열
        receive_channel(B_row,SIZ);  //B의 k번째 행

        for(i = 0; i < SIZ; i++){
            for(j = 0; j < SIZ; j++){
                C[i][j] += A_col[i] * B_row[j];
            }
        }
        free(A_col);
        free(B_row);
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