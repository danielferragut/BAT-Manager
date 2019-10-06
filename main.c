/*      EXPERIMENTO 1
 *   Autores:
 *      Daniel Pereira Ferragut - RA:169488
 *      Lucas Koiti Geminiani Tamanaha - RA:182579
 *
 *      Observações: Devido ao estranho corpotamento das funções pthread_cond e pthread_signal,
 *      foi escolhido trabalhar apenas com um mutex que controla todas as threads.
 * */


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

int car_crossed = 0;        // When a car crosses this var is set
int should_cross[4] = {0,0,0,0};   // Alternative bit mask that warns what thread should cross
int bit_mask[4];            // Bit mask for threads to see other BATS at the crossing
int total_car_number = 0;   // Var to keep track of all cars that passed
int done = 0;               // If this var is set, the line is processed
int starved = 0;            // Var to check if a thread wants to pass its permission to cross
int K = 2;                  // Max number to starve


int main() {
    char    *buffer;
    size_t  n = 1024;
    buffer = malloc(n);
    initialize_thread_array();
    char* k_string;
    int k_int;

/*  For every line BATMAN is going to be called to process it.
 *  It was chosen not to do a fully unblockable reading of the inputs because on
 *  most of the cases, working only with 5 threads, the new BATS would interrupt the current ones
 *  making inconsistent with the experiment's  statement.
 *
 *  It would be possible to process each line on its own (fully unblockable) but then for
 *  each line, 5 threads would be required to process it, not a very scalable solution.
 * */
    while(getline(&buffer, &n, stdin) != -1){
        done = 0;
//        If we want to change K's value
        if (buffer[0] == 'K'){
            k_string = &buffer[2];
            char* new_line;
            k_int = (int)strtol(k_string, &new_line,10);
            K = k_int;
        } else if (buffer[0] == 0x0A){
            break;
        }
        else{
            BAT_manager(buffer);
        }
    }
//    Free everything
    free(buffer);
    BAT_manager_destroy();
    return 0;
}

// Main function for the BAT manager
void* BAT_manager(char* dir_string){
    int i = 0;
//  Every bat is going to the pushed to their respective queues in order of arrival.
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

//    BAT main loop, First we check if new cars can be put at the crossing
//      Second we check if there is a conflict at the crossing.
    while(!done){
        check_for_new_cars();
        check_for_conflict();
    }

//    Lets guarantee that every thread is done by the time BATMAN is done too
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
//                A queue is chosen to cross due to the priority order
                should_cross[i] = 1;

//                Equivalent to pthread_cond_wait, is going to wait for a car to cross
                pthread_mutex_unlock(&mutex);
                while(car_crossed == 0) {}
                pthread_mutex_lock(&mutex);

//                Reseting some vars for next iteration
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
//        Initial print of the problem
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

        /* Main logic of conflict resolution
         *  current_dir is going to find the best car to cross
         *
         *
         *
         * */
        int current_dir = -1;
        int starved_print = 0;
        pthread_mutex_lock(&mutex);
        while (car_crossed == 0){
            current_dir++;
            current_dir = current_dir % 4;
            if (bit_mask[current_dir] != 0){
                should_cross[current_dir] = 1;

//                If this iteration is not a starvation continue
                if (starved_print == 0){
                    printf(" sinalizando %c para ir\n", enum_to_chr(current_dir));
                }

//                Waiting for the car to pass, or for the starvation flag to be set
                pthread_mutex_unlock(&mutex);
                while (car_crossed == 0 && starved == 0){}
                pthread_mutex_lock(&mutex);

//                If there is starvation, continue to next best queue
                if (starved){
                    starved = 0;    // Reset global var
                    starved_print = 1;  // Next iteration is a starvation result, don't print normally
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
//        Reset relevant vars
        should_cross[current_dir] = 0;
        bit_mask[current_dir] = 0;
        car_crossed = 0;
        pthread_mutex_unlock(&mutex);
    }
}

/*  Function that deals with each queue's thread's logic
 * */
void* queue_thread(void* arg){
//    initialization
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

//        Check for conflict and starvation
        int conflict = bit_mask[0] + bit_mask[1] + bit_mask[2] + bit_mask[3];
        if ((queue->size > K) && (current_car->starved_bool == 0) && (conflict > 1)){
            current_car->starved_bool = 1;   //BAT starvation flag gets set
            starved = 1;
        } else{
            cross(current_car);      //BAT gets to cross
            car_crossed = 1;
        }
        should_cross[queue_dir] = 0;  //Warn this thread itself, that a cross should not be made again
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