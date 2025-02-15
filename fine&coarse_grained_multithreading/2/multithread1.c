#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

//coarse grained
//보고서에 스레드 n개일 때 이상적 성능향상과 실제 비교
//스레드가 block 상태로 소모하는 시간이 작으면 작을수록 좋음
//5.74

#define SIZE 700
#define BUFFER_SIZE 10000
#define NUM_THREADS 28

enum STATUS{
    UNSALTED,
    SALTING,
    SALTED
};

typedef struct food {
    int value;
    enum STATUS status;
} FOOD;

struct thread_data {
    struct food **matrix;
    int start_row;
    int end_row;
    int thread_id;
};

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

void cook(struct food tempfood, const char *file_name){
    int result_sum = multiply_matrix_and_sum(tempfood);
    char buffer[BUFFER_SIZE];
    create_concatenated_buf(buffer, result_sum);
    create_concatenated_buf(buffer, result_sum);
    create_concatenated_buf(buffer, result_sum);
    create_concatenated_buf(buffer, result_sum);
    save_buffer_to_file(file_name, buffer);
    //save_buffer_to_file(file_name, buffer);
}

void* thread_task(void* arg){
    struct thread_data *data = (struct thread_data*)arg;
    FOOD **matrix = data->matrix;
    int start_row = data->start_row;
    int end_row = data->end_row;

    char filename[50];
    snprintf(filename,sizeof(filename),"output_thread_%d.txt",data->thread_id);
    
    for(int row = start_row; row < end_row; row++){
        for(int col = 0; col < SIZE; col++){
            matrix[row][col].status = SALTING;
            cook(matrix[row][col],filename);
            matrix[row][col].status = SALTED;
        }
    }
}

int main(){
    srand(time(NULL));
    struct timespec start_time,end_time;

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

    //thread
    pthread_t threads[NUM_THREADS];
    struct thread_data thread_data_arr[NUM_THREADS];

    // struct thread_data {
    //     struct food **matrix;
    //     int start_row;
    //     int end_row;
    //     int thread_id;
    // };

    clock_gettime(CLOCK_MONOTONIC,&start_time);

    for(int t = 0; t < NUM_THREADS; t++){
        thread_data_arr[t].matrix = matrix;
        thread_data_arr[t].start_row = t * 25;
        thread_data_arr[t].end_row = (t + 1) * 25;
        thread_data_arr[t].thread_id = t;
        
        pthread_create(&threads[t],NULL,thread_task,(void*)&thread_data_arr[t]);
    }

    for(int t = 0; t < NUM_THREADS; t++){
        pthread_join(threads[t],NULL);
    }

    clock_gettime(CLOCK_MONOTONIC,&end_time);

    int check = 0;
    for(int row=0; row<SIZE; row++){
        for(int col=0; col<SIZE; col++){
            if(matrix[row][col].status == UNSALTED){
                check++;
            }
        }
    }
    printf("coarse-grain\n");
    printf("UNSALTED : %d\n", check);
    
    double elapsed_time = time_diff_in_seconds(start_time,end_time);
    printf("Execution time: %.2lf seconds.\n",elapsed_time);

    //free matrix
    for(int i = 0; i < SIZE; i++){
        free(matrix[i]);
    }
    free(matrix);

    //printf("hello\n");
    return 0;
}