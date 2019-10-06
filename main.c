#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "bats.h"
#include "queue.h"

void initialize_thread_array();
void BAT_manager_init();
void* BAT_manager(char* dir_string);
void* queue_thread(void* arg);
void check_for_conflict();
void BAT_manager_destroy();


// Global variables
pthread_mutex_t mutex;
pthread_t dir_array[4];
pthread_attr_t attr;
Queue* priority_queue[4];

int car_crossed = 0;
int should_cross[4] = {0,0,0,0};
int bit_mask[4];
int total_car_number = 0;
int done = 0;
int starved = 0;
int K = 2;

int main() {
    char    *buffer;
    size_t  n = 1024;
    buffer = malloc(n);
    initialize_thread_array();
    char* k_string;
    int k_int;
    // While an empty line is not read, continue reading
    while(getline(&buffer, &n, stdin) != 1){
        done = 0;
        if (buffer[0] == 'K'){
            k_string = &buffer[2];
            char* new_line;
            k_int = (int)strtol(k_string, &new_line,10);
            K = k_int;
        }
        else{
            BAT_manager(buffer);
        }
    }
    free(buffer);
    BAT_manager_destroy();
    return 0;
}

// Main function for the BAT manager
void* BAT_manager(char* dir_string){
    int i = 0;
//    Putting all the cars in their respective queues
    char current_dir = dir_string[0];
    BAT* current_car;
    Directions enum_current_dir;
    while(current_dir != '\n'){
        enum_current_dir = chr_to_enum(current_dir);
        current_car = new_car(++total_car_number, enum_current_dir);
        push(priority_queue[enum_current_dir], current_car);
        current_dir = dir_string[++i];
    }

//    Init BAT queues and vars
    BAT_manager_init();
//    BAT main loop, done is set by check_for_conflicts
    while(!done){
        check_for_new_cars();
        check_for_conflict();
    }

    for (i = 0; i < 4; i++) {
        pthread_join(dir_array[i], NULL);
    }

    return NULL;

}

/*  BATMAN function that checks for conflicts or if the process is done
 * */
void check_for_conflict(){
//    There is only a direct conflict if the sum of the bits is bigger than one
    int conflict = bit_mask[0] + bit_mask[1] + bit_mask[2] + bit_mask[3];

    // If there are no cars (conflict == 0), we are done
    if (conflict == 0){
        // Check if there are no more cars left to deal with
        int empty = 0;
        for(int i = 0; i<0; i++){
            empty += priority_queue[i]->size;
        }
        if (empty == 0 ){
            done = 1;
        }
    }
    // If only one car wants to pass
    else if (conflict == 1){
        pthread_mutex_lock(&mutex);
        for(int i = 0; i<4; i++){
            if (bit_mask[i]){
                should_cross[i] = 1;

                pthread_mutex_unlock(&mutex);
                while(car_crossed == 0) {}
                pthread_mutex_lock(&mutex);

                should_cross[i] = 0;
                car_crossed = 0;
                bit_mask[i] = 0;
            }
        }
        pthread_mutex_unlock(&mutex);
    }
//    If there is a conflict
//      For each bit in the bitmask, try to signal the best queue
//      Wait for possible starvation (global var)
//      If no starvation, return from function (only one car pass for each conflict)
//      If there is starvation, try to find next queue
//      Worst case, signal goes to the same BAT, but now it accepts (because only one pass is admitted for each BAT)
    else{
        printf("Impasses: ");
        int first = 1;
        for (int i = 0; i<4; i++){

            if (first && bit_mask[i] != 0){
                printf("%c", enum_to_chr(i));
                first = 0;
            } else if (bit_mask[i]){
                printf(",%c", enum_to_chr(i));
            }
        }

        int current_dir = -1;
        int starved_print = 0;
        pthread_mutex_lock(&mutex);
        while (car_crossed == 0){
            current_dir++;
            current_dir = current_dir % 4;
            if (bit_mask[current_dir] != 0){
                should_cross[current_dir] = 1;
                if (starved_print == 0){
                    printf(" sinalizando %c para ir\n", enum_to_chr(current_dir));
                }

                pthread_mutex_unlock(&mutex);
                while (car_crossed == 0 && starved == 0){}
                pthread_mutex_lock(&mutex);

                if (starved){
                    starved = 0;
                    starved_print = 1;
                    should_cross[current_dir] = 0;
                    BAT* next_bat;
                    BAT* starved_bat = (BAT*)peek(priority_queue[current_dir]);
                    int i = (current_dir+1)%4;
                    while (bit_mask[i] == 0){
                        i++;
                        i = i%4;
                    }
                    next_bat = (BAT*)peek(priority_queue[i]);
                    printf("BAT %d %c cedeu passagem BAT %d %c\n",starved_bat->car_number, enum_to_chr(starved_bat->dir), next_bat->car_number, enum_to_chr(next_bat->dir));
                    continue;
                }
            }
        }
        should_cross[current_dir] = 0;
        bit_mask[current_dir] = 0;
        car_crossed = 0;
        pthread_mutex_unlock(&mutex);
    }
}

/*  Function that deals with each queue's logic
 * */
void* queue_thread(void* arg){
    Directions* dir_ptr = (Directions*) arg;
    Directions queue_dir = *dir_ptr;
    Queue* queue = priority_queue[queue_dir];
    BAT* current_car;
    free(dir_ptr);


//    While this thread's queue is not empty
    while(queue->size != 0){
        current_car = (BAT*)peek(queue);
//        Waiting for permission to cross
        pthread_mutex_lock(&mutex);
        while (should_cross[queue_dir] == 0){
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&mutex);
        }

        int conflict = bit_mask[0] + bit_mask[1] + bit_mask[2] + bit_mask[3];
        if ((queue->size > K) && (current_car->starved_bool == 0) && (conflict > 1)){
            current_car->starved_bool = 1;
            starved = 1;
        } else{
            cross(current_car);
            car_crossed = 1;
        }
        should_cross[queue_dir] = 0;
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void BAT_manager_init(){
    Directions* arg_dir;
    for(int i = 0; i<4; i++){
        bit_mask[i] = 0;
        arg_dir = malloc(sizeof(Directions));
        *arg_dir = (Directions) i;
        pthread_create(&dir_array[i], &attr, queue_thread, arg_dir);
    }
}

void initialize_thread_array(){
    pthread_mutex_init(&mutex, NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);


    Queue* north = create_queue();
    Queue* east = create_queue();
    Queue* south = create_queue();
    Queue* west = create_queue();

    priority_queue[0] = north;
    priority_queue[1] = east;
    priority_queue[2] = south;
    priority_queue[3] = west;

}

void BAT_manager_destroy(){
    pthread_mutex_destroy(&mutex);
    pthread_attr_destroy(&attr);
    for (int i = 0; i < 4; ++i) {
        free(priority_queue[i]);
    }
}