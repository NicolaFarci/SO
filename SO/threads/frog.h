#ifndef FROG_H
#define FROG_H

#include "game.h"
#include "buffer.h"


void *frog_thread(void *arg);
void frog_init(Entity *frog);

#endif