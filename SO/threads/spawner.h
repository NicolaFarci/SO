#ifndef SPAWNER_H
#define SPAWNER_H


#include "game.h"
#include "buffer.h"

typedef struct {
    CircularBuffer *cb;
    RiverLane lane;
} SpawnerArgs;

void create_spawners(CircularBuffer *cb, RiverLane lanes[], pthread_t spawner_tids[], int n_lanes);
void *spawner_thread(void *arg);
void interruptible_sleep(int total_microseconds);


#endif