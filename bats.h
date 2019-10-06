//
// Created by ferragut on 28/09/2019.
//
#ifndef EXP1_STRUCTS_H
#include <ctype.h>
#include "queue.h"
#include <unistd.h>


typedef enum  possible_directions{
    NORTH = 0,
    EAST,
    SOUTH,
    WEST
} Directions;

typedef struct batStruct{
    Directions dir;
    int car_number;
    int starved_bool;
}BAT;

// Functions Prototype
BAT* new_car(int number, Directions dir);
void cross(BAT* current_car);
void arrive(BAT* car, int index);
void check_for_new_cars();
char enum_to_chr(Directions dir);
char chr_to_enum(char dir);


#define EXP1_STRUCTS_H

#endif //EXP1_STRUCTS_H
