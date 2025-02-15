#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define SIZE 700
#define BUFFER_SIZE 10000

enum STATUS{
    UNSALTED,
    SALTING,
    SALTED
};

typedef struct food {
    int value;
    enum STATUS status;
} FOOD;

double time_diff_in_seconds(struct timespec start, struct timespec end){
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int multiply_matrix_and_sum(FOOD tempfood){
    int sum = 0;
    int A[10][10], B[10][10], result[10][10];

    for(int i = 0; i < 10; i++){
        for(int j = 0; j < 10; j++){
            A[i][j] = tempfood.value;
            B[i][j] = tempfood.value;
            result[i][j] = 0;
        }
    }

    for(int i = 0; i < 10; i++){
        for(int j = 0; j < 10; j++){
            for(int k = 0; k < 10; k++){
                result[i][j] = A[i][k] * B[k][j];
            }
            sum += result[i][j];
        }
    }
    return sum;
}

void create_concatenated_buf(char* buffer, int sum){
    char temp_str[12];
    buffer[0] = '\0';

    for(int i = 0; i < 1000; i++){
        snprintf(temp_str,sizeof(temp_str),"%d",sum);
        strcat(buffer,temp_str);
    }
}

void save_buffer_to_file(const char *filename, const char *buffer){
    FILE *file = fopen(filename, "w");
    if(file==NULL){
        printf("cannot open file\n");
        return;
    }
    fprintf(file,"%s",buffer);
    fclose(file);
}

void cook(struct food tempfood){
    struct timespec time1, time2, time3,time4;

    //clock_gettime(CLOCK_MONOTONIC,&time1);
    int result_sum = multiply_matrix_and_sum(tempfood);
    char buffer[BUFFER_SIZE];
    create_concatenated_buf(buffer, result_sum);
    create_concatenated_buf(buffer, result_sum);
    create_concatenated_buf(buffer, result_sum);
    create_concatenated_buf(buffer, result_sum);
    //clock_gettime(CLOCK_MONOTONIC,&time2);
    double elapsed_time1 = time_diff_in_seconds(time1,time2);
    //printf("matrix sum & concatation time: %.8lf seconds.\n",elapsed_time1);


    //clock_gettime(CLOCK_MONOTONIC,&time3);
    save_buffer_to_file("output.txt", buffer);
    //save_buffer_to_file("output.txt", buffer);
    //clock_gettime(CLOCK_MONOTONIC,&time4);
    //double elapsed_time2 = time_diff_in_seconds(time3,time4);
    //printf("i/o time: %.8lf seconds.\n",elapsed_time2);
}

int main(){
    srand(time(NULL));
    struct timespec start_time,end_time;
    clock_gettime(CLOCK_MONOTONIC,&start_time);

    //FOOD 구조체의 2차원 배열
    FOOD **matrix = (FOOD**)malloc(SIZE*sizeof(FOOD*));
    for(int i = 0; i < SIZE; i++){
        matrix[i] = (FOOD*)malloc(SIZE*sizeof(FOOD));
    }
    
    for(int i = 0; i < SIZE; i++){
        for(int j = 0; j < SIZE; j++){
            matrix[i][j].status = UNSALTED;
            matrix[i][j].value = rand() % 10;
        }
    }

    //cook(matrix[0][0]);

    for(int row=0; row<SIZE; row++){
        for(int col=0; col<SIZE; col++){
            matrix[row][col].status = SALTING;
            cook(matrix[row][col]);
            matrix[row][col].status = SALTED;
        }
    }

    int check = 0;
    for(int row=0; row<SIZE; row++){
        for(int col=0; col<SIZE; col++){
            if(matrix[row][col].status == UNSALTED){
                check++;
            }
        }
    }
    printf("single_thread\n");
    printf("UNSALTED : %d\n", check);

    //free matrix
    for(int i = 0; i < SIZE; i++){
        free(matrix[i]);
    }
    free(matrix);
    clock_gettime(CLOCK_MONOTONIC,&end_time);
    double elapsed_time = time_diff_in_seconds(start_time,end_time);
    printf("Execution time: %.2lf seconds.\n",elapsed_time);

    //printf("hello\n");
    return 0;
}