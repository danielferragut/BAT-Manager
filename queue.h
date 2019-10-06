//
// Created by ferragut on 28/09/2019.
//
#ifndef EXP1_QUEUE_H

// Definitions
typedef struct QueueItemStruct{
    void* item;
    struct QueueItemStruct* next;
} QueueItem ;

typedef struct QueueStruct{
    int size;
    QueueItem* firstItem;
} Queue ;

// Global Var
Queue* priority_queue[4];

// Functions
Queue* create_queue();
void push(Queue* queue, void* new_item);
QueueItem* pop(Queue* queue);
int is_empty(Queue* queue);
QueueItem* peek(Queue* queue);
#define EXP1_QUEUE_H

#endif //EXP1_QUEUE_H
