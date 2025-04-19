#include "map.h"
#include "game.h"

pthread_mutex_t render_mutex = PTHREAD_MUTEX_INITIALIZER;

int map[MAP_HEIGHT][MAP_WIDTH];
Hole tane[NUM_HOLES];

// Inizializza la mappa
void init_bckmap(){
    init_pair(2,COLOR_WHITE,COLOR_GREEN);//PRATO
    init_pair(3,COLOR_WHITE,COLOR_BLUE);//ACQUA
    init_pair(4,COLOR_WHITE,COLOR_YELLOW);//TANA LIBERA
    init_pair(5,COLOR_WHITE,COLOR_RED);//TANA OCCUPATA
    init_pair(6,COLOR_GREEN,COLOR_BLUE);//COCCODRILLO
    init_pair(7,COLOR_YELLOW,COLOR_BLUE);//COCCODRILLO CATTIVO

    //associo alla matrice map il valore del color pair apposito
    for(int y = 0; y < MAP_HEIGHT;y++){
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            switch (y<9||y>MAP_HEIGHT-6)
            {
            case true:
                    map[y][x]=2;
                break;
            case false:
                    map[y][x]=3;
                break;
            }
        }
        
    }
}

// Disegna l'intera mappa
void draw_map(WINDOW* win){
    for (int y = 1; y < MAP_HEIGHT-1; y++)
    {
        for (int x = 1; x < MAP_WIDTH-1; x++)
        {
            wattron(win,COLOR_PAIR(map[y][x]));
            mvwaddch(win,y,x,' ');
            wattroff(win,COLOR_PAIR(map[y][x]));
        }
        
    }
    pthread_mutex_lock(&render_mutex);
    wrefresh(win);
    pthread_mutex_unlock(&render_mutex);
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
            for (int dx = 0; dx < FROG_WIDTH; dx++) {
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
        if (frog->x == hole_positions[i]) {
            if (!tane[i].occupied) 
                return i;
            else
                return -1;
        }
    }
    return -1;
}

// Aggiorna la mappa per indicare che la tana è stata occupata
void hole_update(WINDOW *win,int hole_index) {
    tane[hole_index].occupied = true;
    // Ridisegna la tana nella finestra 
    for (int dy = 0; dy < FROG_HEIGHT; dy++) {
        for (int dx = 0; dx < FROG_WIDTH; dx++) {
            map[tane[hole_index].y + dy][tane[hole_index].x + dx] = 5;
            wattron(win, COLOR_PAIR(map[tane[hole_index].y + dy][tane[hole_index].x + dx]));
            mvwaddch(win, tane[hole_index].y + dy, tane[hole_index].x + dx, ' ');
            wattroff(win, COLOR_PAIR(map[tane[hole_index].y + dy][tane[hole_index].x + dx]));
        }
    }
    draw_map(win);
}

// Controlla se tutte le tane sono occupate
bool checkHoles(){
    for (int i = 0; i < NUM_HOLES; i++)
    {
        if (!tane[i].occupied) return false;
    }
    return true;
}