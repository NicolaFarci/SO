#include "grenade.h"
#include "map.h"
#include <stdlib.h>
#include <unistd.h>

void *grenade_left_thread(void *arg) {
    GrenadeArgs *args = (GrenadeArgs*) arg;
    CircularBuffer *buffer = args->buffer;
    Message msg;
    msg.type = MSG_GRENADE_LEFT;
    msg.id = -1;
    msg.entity.width = 1;
    msg.entity.height = 1;
    msg.entity.type = ENTITY_GRENADE;
    msg.entity.dx = args->dx;
    // Inizializza la posizione con quella passata
    msg.entity.x = args->start_x;
    msg.entity.y = args->start_y;


    pthread_mutex_lock(&grenade_mutex);
    active_grenades++;
    pthread_mutex_unlock(&grenade_mutex);

    // Aggiorna la posizione finché non raggiunge il bordo sinistro
    while (msg.entity.x > 0) {
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_PAUSED){
            pthread_mutex_unlock(&game_state_mutex); //sblocca subito per evitare che rimanga bloccato
            usleep(100000); // aspetta un po e ricontrolla se è ancora in pausa
            continue;
        }
        pthread_mutex_unlock(&game_state_mutex);
        
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN){
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);

        usleep(args->speed);
        msg.entity.x += args->dx;
        buffer_push(buffer, msg);
    }

    pthread_mutex_lock(&grenade_mutex);
    active_grenades--;
    pthread_mutex_unlock(&grenade_mutex);
    
    free(args);
    return NULL;
}

void *grenade_right_thread(void *arg) {
    GrenadeArgs *args = (GrenadeArgs*) arg;
    CircularBuffer *buffer = args->buffer;
    Message msg;
    msg.type = MSG_GRENADE_RIGHT;
    msg.id = 1;
    msg.entity.width = 1;
    msg.entity.height = 1;
    msg.entity.type = ENTITY_GRENADE;
    msg.entity.dx = args->dx;
    // Inizializza la posizione con quella passata
    msg.entity.x = args->start_x;
    msg.entity.y = args->start_y;

    pthread_mutex_lock(&grenade_mutex);
    active_grenades++;
    pthread_mutex_unlock(&grenade_mutex);
    
    // Aggiorna la posizione finché non raggiunge il bordo destro
    while (msg.entity.x < MAP_WIDTH) {
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_PAUSED){
            pthread_mutex_unlock(&game_state_mutex); //sblocca subito per evitare che rimanga bloccato
            usleep(100000); // aspetta un po e ricontrolla se è ancora in pausa
            continue;
        }
        pthread_mutex_unlock(&game_state_mutex);

        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN){
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);

        usleep(args->speed);
        msg.entity.x += args->dx; 
        buffer_push(buffer, msg);
    }

    pthread_mutex_lock(&grenade_mutex);
    active_grenades--;
    pthread_mutex_unlock(&grenade_mutex);

    free(args);
    return NULL;
}

// Disegna la granata sullo schermo
void draw_grenade(WINDOW *win, Entity *grenade) {
    if(grenade->x>0 && grenade->x<MAP_WIDTH-1){
        wattron(win,COLOR_PAIR(map[grenade->y][grenade->x]));
        mvwaddch(win,grenade->y, grenade->x, '*');
        wattroff(win,COLOR_PAIR(map[grenade->y][grenade->x]));
        pthread_mutex_lock(&render_mutex);
        wrefresh(win);
        pthread_mutex_unlock(&render_mutex);
    }
    
}
void clear_grenade(WINDOW *win, Entity *grenade) {
    if(grenade->x>0 && grenade->x<MAP_WIDTH-1){
        wattron(win,COLOR_PAIR(map[grenade->y][grenade->x]));
        mvwaddch(win,grenade->y, grenade->x, ' ');
        wattroff(win,COLOR_PAIR(map[grenade->y][grenade->x]));
        pthread_mutex_lock(&render_mutex);
        wrefresh(win);
        pthread_mutex_unlock(&render_mutex);
    }
}