#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

//fine grained
//보고서에 스레드 n개일 때 이상적 성능향상과 실제 비교
//disk i/o에 시간이 많이 걸리는 경우 유용함. 무조건 coarse보다 좋은게아님

//코어 수보다 스레드 수를 더 많이 만들어서 
//어떤 스레드가 block 상태가 되면
//정해진 스케줄링 알고리즘대로 코어가 다른 스레드에 알아서 배정되게 만듦

//코어보다 스레드 수가 많아질 때
//코어를 배정하는 스케줄링 알고리즘 필요->오버헤드

//또다른 방법
//thread pool

#define SIZE 700
#define BUFFER_SIZE 10000
#define NUM_THREADS 56

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
    int start_col;

    int thread_id;
};

int salted_sum = 0;

sem_t bin_sem;

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
    //save_buffer_to_file(file_name, buffer);
    save_buffer_to_file(file_name, buffer);
}

void* thread_task(void* arg){
    struct thread_data *data = (struct thread_data*)arg;
    FOOD **matrix = data->matrix;
    int row = data->start_row;
    int col = data->start_col;
    int thread_id = data->thread_id;

    char filename[50];
    snprintf(filename,sizeof(filename),"output_thread_%d.txt",data->thread_id);

    int go;
    int current_salted_sum;

    unsigned end = SIZE*SIZE;

    // while(1){
    //     current_salted_sum = __sync_fetch_and_add(&salted_sum, 0);
    //     if(current_salted_sum >= end)
    //         break;

    //     //printf("thread %d check matrix[%d][%d]\n",thread_id,row,col);
    //     //unsalted임을 확인하는 부분을 보호해줘야됨
    //     //__sync_bool_compare_and_swap(&matrix[row][col].status, UNSALTED, SALTING); 도 가능
    //     sem_wait(&bin_sem);
    //     if(matrix[row][col].status == UNSALTED){
    //         matrix[row][col].status = SALTING;
    //         go = 1;
    //     }
    //     else{
    //         go = 0;
    //     }
    //     sem_post(&bin_sem);

    //     if(go == 1){
    //         //printf("cook matrix[%d][%d]\n",row,col);
    //         cook(matrix[row][col],filename);
    //         __sync_fetch_and_add(&salted_sum,1); //atomic
    //         matrix[row][col].status = SALTED;
    //         //find_other dish - algorithm
    //         if(col == SIZE - 1){
    //             if(row == SIZE - 1)
    //                 row = 0;
    //             else
    //                 row++;
    //             col = 0;
    //         }
    //         else{
    //             col++;
    //         }
    //     }
    //     else{
    //         //find_other dish - algorithm
    //         if(col == SIZE - 1){
    //             if(row == SIZE - 1)
    //                 row = 0;
    //             else
    //                 row++;
    //             col = 0;
    //         }
    //         else{
    //             col++;
    //         }
    //     }
    // }

    //세마포 대신 comp&swap 사용 - 성능차이 거의 x
    while(1){
        current_salted_sum = __sync_fetch_and_add(&salted_sum, 0);
        if(current_salted_sum >= end)
            break;

        //printf("thread %d check matrix[%d][%d]\n",thread_id,row,col);
        //unsalted임을 확인하는 부분을 보호해줘야됨
        if(__sync_bool_compare_and_swap(&matrix[row][col].status, UNSALTED, SALTING)){
            cook(matrix[row][col],filename);
            __sync_fetch_and_add(&salted_sum,1); //atomic
            matrix[row][col].status = SALTED;
            //find_other dish - algorithm
            if(col == SIZE - 1){
                if(row == SIZE - 1)
                    row = 0;
                else
                    row++;
                col = 0;
            }
            else{
                col++;
            }
        }
        else{
            //find_other dish - algorithm
            if(col == SIZE - 1){
                if(row == SIZE - 1)
                    row = 0;
                else
                    row++;
                col = 0;
            }
            else{
                col++;
            }
        }
    }

    pthread_exit(NULL);
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

    clock_gettime(CLOCK_MONOTONIC,&start_time);

    //thread
    pthread_t threads[NUM_THREADS];
    struct thread_data thread_data_arr[NUM_THREADS];

    int res;
    //res = sem_init(&bin_sem, 0,1);

    for(int t = 0; t < NUM_THREADS; t++){
        thread_data_arr[t].matrix = matrix;
        thread_data_arr[t].start_row = rand() % SIZE;
        thread_data_arr[t].start_col = rand() % SIZE;
        thread_data_arr[t].thread_id = t;
        
        pthread_create(&threads[t],NULL,thread_task,(void*)&thread_data_arr[t]);
    }

    for(int t = 0; t < NUM_THREADS; t++){
        pthread_join(threads[t],NULL);
    }

    //sem_destroy(&bin_sem);

    clock_gettime(CLOCK_MONOTONIC,&end_time);

    int check = 0;
    for(int row=0; row<SIZE; row++){
        for(int col=0; col<SIZE; col++){
            if(matrix[row][col].status == UNSALTED){
                check++;
            }
        }
    }
    printf("fine-grain\n");
    printf("salted_num : %d * %d = %d\n",SIZE,SIZE,salted_sum);
    printf("UNSALTED : %d\n", check);

    //free matrix
    for(int i = 0; i < SIZE; i++){
        free(matrix[i]);
    }
    free(matrix);
    

    
    double elapsed_time = time_diff_in_seconds(start_time,end_time);
    printf("Execution time: %.2lf seconds.\n",elapsed_time);

    //printf("hello\n");
    return 0;
}