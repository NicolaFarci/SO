#ifndef MAP_H
#define MAP_H

#include "game.h"
#include "buffer.h"

#define BOTTOM_SIDEWALK 4

#define NUM_HOLES 5

// Costanti per le posizioni delle tane
#define HOLE_Y 1
#define HOLE_X1 6  
#define HOLE_X2 21 
#define HOLE_X3 36
#define HOLE_X4 51
#define HOLE_X5 66


// Struttura per rappresentare una tana
typedef struct {
    int x;
    int y;
    bool occupied;
} Hole;

extern int map[MAP_HEIGHT][MAP_WIDTH];
extern Hole tane[NUM_HOLES];

void init_bckmap();
void draw_map();
void init_holes_positions();
void init_map_holes();
int check_hole_reached(Entity *frog);
void hole_update(int hole_index);
bool checkHoles();


#endif 
