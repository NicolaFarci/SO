#include "frog.h"
#include "game.h"
#include "buffer.h"

void *frog_thread(void *arg) {
    CircularBuffer *cb = arg;
    nodelay(stdscr,FALSE);
    keypad(stdscr, TRUE);
    Message msg;
    msg.type = MSG_FROG_UPDATE;
    while (1) {
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN){
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);
        msg.entity.dx = 0;
        msg.entity.dy = 0;
        //leggo i tasti premuti dall'utente
        int ch = getch();
        switch (ch) {
            case 'w':
            case 'W':
            case KEY_UP:
                msg.entity.dy = -VERTICAL_JUMP; // salta in alto
                break;
            case 's':
            case 'S':
            case KEY_DOWN:
                msg.entity.dy = +VERTICAL_JUMP; // salta in basso
                break;
            case 'a':
            case 'A':
            case KEY_LEFT:
                msg.entity.dx = -HORIZONTAL_JUMP; // salta a sinistra
                break;
            case 'd':
            case 'D':
            case KEY_RIGHT:
                msg.entity.dx = +HORIZONTAL_JUMP; // salta a destra
                break;
            case ' ':
                { // lancia una granata
                    Message gmsg;
                    gmsg.type = MSG_GRENADE_SPAWN;
                    //invia il messaggio di spawn della granata
                    buffer_push(cb, gmsg);
                }
                break;
            case 'p':
            case 'P':
                { // toggle pausa
                    Message pmsg;
                    pmsg.type = MSG_PAUSE;
                    //invia il messaggio di pausa
                    buffer_push(cb, pmsg);
                }
                break;
            default:
                break;
        }
        //manda solo se ho mosso la rana
        if (msg.entity.dx != 0 || msg.entity.dy != 0) {            
            buffer_push(cb, msg);
        }
        usleep(30000);
    }
    pthread_exit(NULL);
}

void frog_init(Entity *frog) {
    // Inizializza le proprietà della rana
    frog->x = (MAP_WIDTH - FROG_WIDTH) / 2;
    frog->y = MAP_HEIGHT - FROG_HEIGHT;
    frog->width = FROG_WIDTH;
    frog->height = FROG_HEIGHT;
    frog->type = ENTITY_FROG;
    // Inizializza lo sprite della rana
    strcpy(frog->sprite[0], "vOv");
    strcpy(frog->sprite[1], "wUw");
}
