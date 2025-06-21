#include "timer.h"

//questo thread invia un MSG_TIMER_TICK ogni secondo nel buffer condiviso
void *timer_thread(void *arg) {
    CircularBuffer *cb = (CircularBuffer *)arg;
    Message msg;
    msg.type = MSG_TIMER_TICK;

    while (1) {
        pthread_mutex_lock(&pause_mutex);
        while (paused) {
            // Qui il produttore si sospende, non produce nulla
            pthread_cond_wait(&pause_cond, &pause_mutex);
        }
        pthread_mutex_unlock(&pause_mutex);
        buffer_push(cb, msg); 
        sleep(1);
    }
    return NULL;
}