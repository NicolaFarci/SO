#include "crocodile.h"
#include "grenade.h"
#include "map.h"
#include "game.h"
#include <stdlib.h>
#include <unistd.h>

void init_lanes(RiverLane lanes[]) {
    int num = rand()%100;
    if(num%2==0){
        lanes[0].direction = 1;
    }else{
        lanes[0].direction = -1;
    }
    lanes[0].speed = 200000;
    lanes[0].y = MAP_HEIGHT - 7;
    lanes[0].index = 0;
    lanes[0].movements = 0;
    lanes[0].movements_to_spawn = 15; // Initial value
    
    // Alternate directions for subsequent lanes
    for (int i = 1; i < NUM_RIVER_LANES; i++) {
        lanes[i].direction = -lanes[i-1].direction;
        lanes[i].speed = lanes[i-1].speed - 10000;
        lanes[i].y = lanes[i-1].y - FROG_HEIGHT;
        lanes[i].index = i;
        lanes[i].movements = 0;
        lanes[i].movements_to_spawn = 15 + (2 * i); // Different thresholds for each lane
    }
}

void crocodile_init(Entity *crocodile, RiverLane *lane) {
    crocodile->type = ENTITY_CROCODILE;
    crocodile->width = CROCODILE_WIDTH;
    crocodile->height = CROCODILE_HEIGHT;
    crocodile->y = lane->y;
    crocodile->dx = lane->direction;
    crocodile->speed = lane->speed;
    
    int randnum= rand()%100;
    if(randnum<15){
        crocodile->is_badcroc=true;
        crocodile->cooldown= 20+randnum;
    }else{
        crocodile->is_badcroc=false;
        crocodile->cooldown=-1;
    }
    
    // Initial x position based on direction
    if (lane->direction > 0) {
        crocodile->x = 1 - CROCODILE_WIDTH;
    } else {
        crocodile->x = MAP_WIDTH - 1;
    }

    // Crocodile sprite
    char sprite[CROCODILE_HEIGHT][CROCODILE_WIDTH] = {
        {'<', 'R', 'R', 'R', 'R','R','R','R','>'},
        {'<', 'R', 'R', 'R', 'R','R','R','R','>'}
    };

    for (int i = 0; i < CROCODILE_HEIGHT; i++) {
        for (int j = 0; j < CROCODILE_WIDTH; j++) {
            crocodile->sprite[i][j] = sprite[i][j];
        }
    }
}

void *crocodile_thread(void *arg) {
    CrocodileArgs *args = (CrocodileArgs*) arg;
    CircularBuffer *buffer = args->buffer;
    RiverLane *lane = args->lane;

    Message msg;
    Entity crocodile;
    
    // Initialize crocodile
    crocodile_init(&crocodile, lane);
    
    // Prepare message
    msg.type = MSG_CROC_SPAWN;
    msg.id = lane->index;
    msg.entity = crocodile;

    buffer_push(buffer, msg);

    msg.type = MSG_CROC_UPDATE;

    while (1) {
        // Check if game is paused
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_PAUSED) {
            pthread_mutex_unlock(&game_state_mutex);
            usleep(100000);
            continue;
        }
        pthread_mutex_unlock(&game_state_mutex);
        
        // Check if game is quitting
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN) {
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);
        
        // Sleep based on speed
        usleep(crocodile.speed);
        
        // Update x position based on lane direction
        crocodile.x += crocodile.dx;
        
        // Check for boundaries
        if ((crocodile.dx > 0 && crocodile.x > MAP_WIDTH) ||
            (crocodile.dx < 0 && crocodile.x < 1 - crocodile.width)) {
            break;
        }
        
        // Prepare and send message
        msg.entity = crocodile;
        buffer_push(buffer, msg);
    }

    msg.type = MSG_CROC_DESPAWN;
    msg.entity = crocodile;
    buffer_push(buffer, msg);
    
    return NULL;
}

void *crocodile_projectile_thread(void *arg) {
    GrenadeArgs *args = (GrenadeArgs*) arg;
    CircularBuffer *buffer = args->buffer;
    Message msg;
    
    Entity proiettile;
    proiettile.cooldown=-1;
    proiettile.dx=args->dx;
    proiettile.height=1;
    proiettile.is_badcroc=false;
    proiettile.speed=args->speed;
    proiettile.sprite[0][0]='*';
    proiettile.type=ENTITY_CROCODILEPROJ;
    proiettile.width=1;
    proiettile.x=args->start_x;
    proiettile.y=args->start_y;

    msg.entity=proiettile;
    msg.id = +1;
    msg.type=MSG_CROC_PROJECTILE;
    buffer_push(buffer,msg);

    msg.id = 0;

    while(proiettile.x>0&&proiettile.x<MAP_WIDTH){
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
        proiettile.x += proiettile.dx;
        msg.entity = proiettile;
        buffer_push(buffer, msg);
    }

    msg.id=-1;
    buffer_push(buffer, msg);


    return NULL;
}

void draw_crocodile(WINDOW *win, Entity *crocodile) {
    int num;
    if (crocodile->is_badcroc){
        num=7;
    }else{
        num=6;
    }
    for (int i = 0; i < crocodile->height; i++) {
        for (int j = 0; j < crocodile->width - 1; j++) {
            if(crocodile->y + i < MAP_HEIGHT && crocodile->y + i > 0 &&
                crocodile->x + j < MAP_WIDTH - 1 && crocodile->x + j > 0){
                    wattron(win, COLOR_PAIR(num));
                    mvwaddch(win, crocodile->y + i, crocodile->x + j, crocodile->sprite[i][j]);
                    wattroff(win, COLOR_PAIR(num));
                }
        }
    }
    wrefresh(win);
}

void clear_crocodile(WINDOW *win, Entity *crocodile) {
    for (int i = 0; i < crocodile->height; i++) {
        for (int j = 0; j < crocodile->width; j++) {
            if(crocodile->y + i < MAP_HEIGHT && crocodile->y + i > 0 &&
                crocodile->x + j < MAP_WIDTH - 1 && crocodile->x + j > 0){
                    wattron(win, COLOR_PAIR(6));
                    mvwaddch(win, crocodile->y + i, crocodile->x + j, ' ');
                    wattroff(win, COLOR_PAIR(6));
                }
        }
    }
    wrefresh(win);
}
