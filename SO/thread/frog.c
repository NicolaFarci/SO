#include "frog.h"
#include "game.h"
#include "buffer.h"
#include "map.h"
#include "consumer.h"
#include "grenade.h"



void *frog_thread(void *arg) {
    FrogArgs *args = (FrogArgs *)arg;
    CircularBuffer *buffer = args->buffer;
    WINDOW *game_win = args->win;
    nodelay(game_win,FALSE);
    Entity frog;
    frog_init(&frog);
    int ch;
    Message msg;
    bool can_shoot;
    int old_x, old_y;

    msg.type = MSG_FROG_UPDATE;
    msg.entity = frog;
    buffer_push(buffer, msg);

    while (1) {
        // Controllo eventuale reset
        pthread_mutex_lock(&reset_mutex);
        if (round_reset_flag) {
            frog.x = (MAP_WIDTH - frog.width) / 2;
            frog.y = MAP_HEIGHT - frog.height - 1;
            round_reset_flag = 0;
        }
        pthread_mutex_unlock(&reset_mutex);
        // Controlla se si vuole uscire dal gioco o se il giocatore ha vinto
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN){
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);
        
        ch = wgetch(game_win);
        old_x=frog.x;
        old_y=frog.y;

        // Controlla Pausa o Quit del gioco
        if (ch == 'p' || ch == 'P'){
            pthread_mutex_lock(&game_state_mutex);
            if (game_state == GAME_RUNNING)
                game_state = GAME_PAUSED;
            else if (game_state == GAME_PAUSED)
                game_state = GAME_RUNNING;
            pthread_mutex_unlock(&game_state_mutex);
            continue; 
            
        } else if (ch == 'q' || ch == 'Q'){
            pthread_mutex_lock(&game_state_mutex);
            game_state = GAME_QUITTING;
            pthread_mutex_unlock(&game_state_mutex);
            continue; 
        }
        //INVIA UN MEX AL CONSUMER CHE DICE COSA FARE
        if(game_state != GAME_PAUSED){
            switch (ch){
            case KEY_UP:
            case KEY_DOWN:
            case KEY_LEFT:
            case KEY_RIGHT:
                msg.type=MSG_FROG_UPDATE;
                msg.id=ch;
                break;
            case ' ':
                pthread_mutex_lock(&grenade_mutex);
                can_shoot=(active_grenades==0);
                pthread_mutex_unlock(&grenade_mutex);
                if(can_shoot){
                    msg.type = MSG_GRENADE_SPAWN;
                    msg.id = 0;  // Not used for grenades
                    msg.entity = frog;  // Pass frog position for grenade spawn
                }
                break;
            }
            buffer_push(buffer, msg);
        }
    }
    return NULL;
}

void frog_init(Entity *frog) {
    frog->x = (MAP_WIDTH - FROG_WIDTH ) / 2 ;
    frog->y = MAP_HEIGHT - FROG_HEIGHT - 1;
    frog->width = FROG_WIDTH;
    frog->height = FROG_HEIGHT;
    frog->type = ENTITY_FROG;

    frog->has_shot=false;
    frog->is_badcroc=false;
    frog->dx=0;
    frog->speed=0;
    
    // Forma della rana
    char sprite[FROG_HEIGHT][FROG_WIDTH] = {
        {'v', 'O', 'v'},
        {'w', 'U', 'w'},
    };

    for (int i = 0; i < FROG_HEIGHT; i++) {
        for (int j = 0; j < FROG_WIDTH; j++) {
            frog->sprite[i][j] = sprite[i][j];
        }
    }
}



// Disegna la rana sullo schermo
void draw_frog(WINDOW *win, Entity *frog) {
    for (int i = 0; i < FROG_HEIGHT; i++) {
        for (int j = 0; j < FROG_WIDTH; j++) {
            wattron(win,COLOR_PAIR(map[frog->y + i][frog->x + j]));
            mvwaddch(win,frog->y + i, frog->x + j, frog->sprite[i][j]);
            wattroff(win,COLOR_PAIR(map[frog->y + i][frog->x + j]));
        }
    }
    wrefresh(win);
}
void clear_frog(WINDOW *win, Entity *frog) {
    for (int i = 0; i < frog->height; i++) {
        for (int j = 0; j < frog->width; j++) {
            wattron(win,COLOR_PAIR(map[frog->y + i][frog->x + j]));
            mvwaddch(win,frog->y + i, frog->x + j,' ');
            wattroff(win,COLOR_PAIR(map[frog->y + i][frog->x + j]));
        }
    }
    wrefresh(win);
}