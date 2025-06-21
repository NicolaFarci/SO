#include "game.h"
#include "grenade.h"
#include "map.h"


void *grenade_thread(void *varg) {
    GrenadeArgs *args = varg;
    CircularBuffer *cb = args->cb;
    Message msg;
    //setup iniziale della granata
    Entity grenade;
    grenade.type = ENTITY_GRENADE;
    grenade.width = 1;
    grenade.height = 1;
    grenade.x = args->start_x;
    grenade.y = args->start_y;
    grenade.dx = args->dx;
    grenade.speed = 40000;
    grenade.sprite[0][0] = 'O';
    free(args);
    pthread_t my_tid = pthread_self(); // ottengo il pid del processo corrente
    
    //manda posizione della granata finchè non esce dallo schermo
    while (1) {
        pthread_mutex_lock(&pause_mutex);
        while (paused) {
            //qui il produttore si sospende, non produce nulla
            pthread_cond_wait(&pause_cond, &pause_mutex);
        }
        pthread_mutex_unlock(&pause_mutex);
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN){
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);
        msg.type = MSG_GRENADE_UPDATE;
        msg.entity = grenade;
        msg.id = my_tid;
        buffer_push(cb, msg);
        //sposto la granata in base alla sua direzione
        grenade.x += grenade.dx;
        //se la granata esce dallo schermo, termina il ciclo
        if (grenade.x <= 0 || grenade.x >= MAP_WIDTH) break;
        
        usleep(grenade.speed);
    }

    //messaggio di despawn
    msg.type = MSG_GRENADE_DESPAWN;
    msg.entity = grenade;
    msg.id = my_tid;
    buffer_push(cb, msg);


    pthread_exit(NULL);
}

void draw_grenade(Entity *grenade) {
    if (grenade->x >= 0 && grenade->x < MAP_WIDTH && grenade->y >= 0 && grenade->y < MAP_HEIGHT) {
        attron(COLOR_PAIR(map[grenade->y][grenade->x]));
        mvaddch(grenade->y, grenade->x, grenade->sprite[0][0]);
        attroff(COLOR_PAIR(map[grenade->y][grenade->x]));
    }
}

void clear_grenade(Entity *grenade) {
    if (grenade->x >= 0 && grenade->x < MAP_WIDTH && grenade->y >= 0 && grenade->y < MAP_HEIGHT) {
        attron(COLOR_PAIR(map[grenade->y][grenade->x]));
        mvaddch(grenade->y, grenade->x, ' ');
        attroff(COLOR_PAIR(map[grenade->y][grenade->x]));
    }
}

