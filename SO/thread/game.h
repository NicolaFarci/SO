#ifndef GAME_H
#define GAME_H

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


//Dimensione dell'area di gioco
#define MAP_HEIGHT 40
#define MAP_WIDTH 77
#define INFO_HEIGHT 3

#define NUM_OPTIONS 3

//Dimensioni del buffer condiviso
#define BUFFER_SIZE 32

//Dimensioni e salti della rana
#define FROG_WIDTH 3
#define FROG_HEIGHT 2
#define HORIZONTAL_JUMP FROG_WIDTH
#define VERTICAL_JUMP   FROG_HEIGHT

// Parametri di gioco
#define NUM_LIVES 3
#define INITIAL_SCORE 0
#define ROUND_TIME 50
#define NUM_HOLES 5


// Dimensioni massime della matrice sprite
#define SPRITE_ROWS 2
#define SPRITE_COLS 16

//Tipi di entità
typedef enum {
    ENTITY_FROG,
    ENTITY_GRENADE
    // Altre entità possono essere aggiunte in futuro
} EntityType;

// Struttura per le entità
typedef struct {
    EntityType type;
    int x;
    int y;
    int width;
    int height;
    int dx; // direzione
    int speed;
    char sprite[SPRITE_ROWS][SPRITE_COLS];
} Entity;

//Tipi di messaggi
typedef enum {
    MSG_FROG_UPDATE,
    MSG_TIMER_TICK,
    MSG_GRENADE_LEFT,
    MSG_GRENADE_RIGHT,
} MessageType;

//Struttura del messaggio inviato
typedef struct {
    MessageType type;
    int id; // 0 per la rana
    Entity entity;
} Message;

// Struttura del buffer circolare
typedef struct {
    Message buffer[BUFFER_SIZE];
    int in;
    int out;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t full;
    pthread_cond_t empty;
} CircularBuffer;

//Stato attuale del gioco
typedef enum {
    GAME_RUNNING,
    GAME_PAUSED,
    GAME_QUITTING,
    GAME_WIN
} GameState;

extern int round_reset_flag;
extern pthread_mutex_t reset_mutex;

extern int active_grenades;
extern pthread_mutex_t grenade_mutex;

extern int game_state;
extern pthread_mutex_t game_state_mutex;

extern int score;

void show_instructions(WINDOW *menu_win);
void exit_program(WINDOW *menu_win);
void start_game();
void game_state_win();
void game_over();
void restart_game();

#endif
