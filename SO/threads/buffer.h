#ifndef BUFFER_H
#define BUFFER_H

#include <semaphore.h>
#include <pthread.h>
#include "game.h"

#define BUFFER_SIZE 64

typedef struct {
    Message buffer[BUFFER_SIZE];
    int in, out,count; // in/out index e contatore
    sem_t empty;// quante celle vuote
    sem_t full; // quante celle piene
    pthread_mutex_t mutex;// per proteggere in/out
} CircularBuffer;

// inizializza buffer
void buffer_init(CircularBuffer *cb);
// distrugge le primitive
void buffer_destroy(CircularBuffer *cb);
// mette un messaggio, blocca se pieno
void buffer_push(CircularBuffer *cb, Message msg);
// preleva un messaggio, blocca se vuoto
Message buffer_pop(CircularBuffer *cb);
bool buffer_try_pop(CircularBuffer *cb, Message *msg);

#endif