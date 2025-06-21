#include "map.h"
#include "game.h"

int map[MAP_HEIGHT][MAP_WIDTH];
Hole tane[NUM_HOLES];

// Inizializza la mappa
void init_bckmap() {
    init_pair(2, COLOR_WHITE, COLOR_GREEN);  // PRATO
    init_pair(3, COLOR_WHITE, COLOR_BLUE);   // ACQUA
    init_pair(4, COLOR_WHITE, COLOR_YELLOW); // TANA LIBERA
    init_pair(6, COLOR_GREEN, COLOR_BLUE);   // COCCODRILLO
    init_pair(7, COLOR_YELLOW, COLOR_BLUE);  // COCCODRILLO CATTIVO
    init_pair(8,COLOR_RED, COLOR_BLUE);  // PROIETTILE
    

    // associo alla matrice map il valore del color pair apposito
    for(int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map[y][x] = (y < 7 || y > MAP_HEIGHT-5) ? 2 : 3;
        }
    }
}

// Disegna l'intera mappa
void draw_map() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            attron(COLOR_PAIR(map[y][x]));
            mvaddch(y, x, ' ');
            attroff(COLOR_PAIR(map[y][x]));
        }
    }
}


//Viene riempito l'array di tane
void init_holes_positions() {
    int hole_positions[NUM_HOLES] = {HOLE_X1, HOLE_X2, HOLE_X3, HOLE_X4, HOLE_X5};
    for (int i = 0; i < NUM_HOLES; i++) {
        tane[i].x = hole_positions[i];
        tane[i].y = HOLE_Y;
        tane[i].occupied = false;
    }
}

// Inserisce le tane nella matrice di interi rappresentante la mappa
void init_map_holes() {
    for (int i = 0; i < NUM_HOLES; i++) {
        for (int dy = 0; dy < FROG_HEIGHT; dy++) {
            for (int dx = 0; dx < FROG_WIDTH*2; dx++) {
                map[tane[i].y + dy][tane[i].x + dx] = 4;
            }
        }
    }
}
// trova l'indice della tana raggiunta
int check_hole_reached(Entity *frog) {
    if (frog->y != HOLE_Y)
        return -1;

    int hole_positions[NUM_HOLES] = { HOLE_X1, HOLE_X2, HOLE_X3, HOLE_X4, HOLE_X5 };
    for (int i = 0; i < NUM_HOLES; i++) {
        if (frog->x == hole_positions[i] || frog->x == hole_positions[i] + 1 || frog->x == hole_positions[i] + 2 || frog->x == hole_positions[i] + frog->width){
            if (!tane[i].occupied) 
                return i;
            else
                return -1;
        }
    }
    return -1;
}

// Aggiorna la mappa per indicare che la tana è stata occupata
void hole_update(int hole_index) {
    tane[hole_index].occupied = true;
    // Ridisegna la tana 
    for (int dy = 0; dy < FROG_HEIGHT; dy++) {
        for (int dx = 0; dx < FROG_WIDTH*2; dx++) {
            map[tane[hole_index].y + dy][tane[hole_index].x + dx] = 2;
            attron(COLOR_PAIR(map[tane[hole_index].y + dy][tane[hole_index].x + dx]));
            mvaddch(tane[hole_index].y + dy, tane[hole_index].x + dx, ' ');
            attroff(COLOR_PAIR(map[tane[hole_index].y + dy][tane[hole_index].x + dx]));
        }
    }
}

// Controlla se tutte le tane sono occupate
bool checkHoles(){
    for (int i = 0; i < NUM_HOLES; i++)
    {
        if (!tane[i].occupied) return false;
    }
    return true;
}