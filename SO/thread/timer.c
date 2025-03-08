#include "timer.h"
#include "buffer.h"


void *timer_thread(void *arg) {
    TimerArgs *args = (TimerArgs *)arg;
    CircularBuffer *buffer = args->buffer;
    Message msg;

    while (1) {
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
        
        sleep(1);
        msg.type = MSG_TIMER_TICK;
        buffer_push(buffer, msg);
    }
    return NULL;
}