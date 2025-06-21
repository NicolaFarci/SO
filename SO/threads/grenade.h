#ifndef GRENADE_H
#define GRENADE_H

#include "game.h"
#include "buffer.h"

#define MAX_GRENADES 2

//struttura per passare gli argomenti alla grenade_thread
typedef struct {
    CircularBuffer *cb;
    int start_x;
    int start_y;
    int dx;
    int slot; // per identificare la granata (0 o 1)
} GrenadeArgs;

void *grenade_thread(void *arg);
void draw_grenade(Entity *grenade);
void clear_grenade(Entity *grenade);

#endif
