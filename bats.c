//
// Created by ferragut on 06/10/2019.
//

#include "bats.h"
#include <stdlib.h>
#include <stdio.h>

extern int bit_mask[];


// Functions that creates a new car
BAT* new_car(int number, Directions dir){
    BAT* car = malloc(sizeof(BAT));
    car->dir = dir;
    car->car_number = number;
    car->starved_bool = 0;
    return car;
}

// Function that triggers everytime a car crosses
void cross(BAT* current_car){
    printf("BAT %d %c saiu do cruzamento\n", current_car->car_number, enum_to_chr(current_car->dir));
    switch (current_car->dir){
        case NORTH:
            pop(priority_queue[0]);
            break;
        case EAST:
            pop(priority_queue[1]);
            break;
        case SOUTH:
            pop(priority_queue[2]);
            break;
        case WEST:
            pop(priority_queue[3]);
            break;
    }
}

// Function that is triggred everytime a car arrives
void arrive(BAT* car, int index){
    bit_mask[index] = 1;
    printf("BAT %d %c chegou no cruzamento\n", car->car_number, enum_to_chr(car->dir));
}


/*  Checks if there are any cars that want to cross
 * */
void check_for_new_cars(){
    for(int i = 0; i<4; i++){
        if(priority_queue[i]->size != 0){
            if (bit_mask[i] == 0){
                BAT* car = (BAT*)peek(priority_queue[i]);
                arrive(car, i);
            }
        }
    }
}

// Transforms a enum Directions to a char
char enum_to_chr(Directions dir){
    char chr_dir;
    switch (dir){
        case NORTH:
            chr_dir = 'N';
            break;
        case EAST:
            chr_dir = 'E';
            break;
        case SOUTH:
            chr_dir = 'S';
            break;
        case WEST:
            chr_dir = 'W';
            break;
    }
    return chr_dir;
}

// Transforms a char to a Direction Enum
char chr_to_enum(char dir){
    Directions enum_dir;
    dir = toupper(dir);
    switch (dir){
        case 'N':
            enum_dir = NORTH;
            break;
        case 'E':
            enum_dir = EAST;
            break;
        case 'S':
            enum_dir = SOUTH;
            break;
        case 'W':
            enum_dir = WEST;
            break;
    }
    return enum_dir;
}
