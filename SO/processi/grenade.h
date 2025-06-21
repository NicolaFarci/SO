#ifndef GRENADE_H
#define GRENADE_H

#include "game.h"

#define MAX_GRENADES 2


void grenade_process(int fd_write,int start_x, int start_y, int dx);
void draw_grenade(Entity *grenade);
void clear_grenade(Entity *grenade);

#endif
