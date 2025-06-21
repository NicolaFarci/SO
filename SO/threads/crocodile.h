#ifndef CROCODILE_H
#define CROCODILE_H

#include "game.h"
#include "buffer.h"
#include <sys/types.h>

typedef struct {
    CircularBuffer *cb;
    RiverLane lane;
} CrocArgs;

typedef struct {
    CircularBuffer *cb;
    int start_x, start_y, dx;
} ProjectileArgs;

void init_lanes(RiverLane lanes[]);
void *crocodile_thread(void *arg);
void crocodile_init(Entity *crocodile, RiverLane *lane);
void draw_crocodile(Entity *crocodile);
void *projectile_thread(void *arg);
void draw_projectile(Entity *projectile);
void clear_projectile(Entity *projectile);

#endif
