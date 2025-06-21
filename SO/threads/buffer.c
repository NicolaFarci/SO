#include "buffer.h"

//inizializza buffer circolare
void buffer_init(CircularBuffer *cb) {
    cb->in = 0;
    cb->out = 0;
    //semaforo empty: numero di slot liberi inizialmente = BUFFER_SIZE
    sem_init(&cb->empty,0,BUFFER_SIZE);
    //semaforo full: numero di slot occupati inizialmente = 0
    sem_init(&cb->full,0,0);
    //mutex per in/out
    pthread_mutex_init(&cb->mutex, NULL);
}

void buffer_destroy(CircularBuffer *cb) {
    sem_destroy(&cb->empty);
    sem_destroy(&cb->full);
    pthread_mutex_destroy(&cb->mutex);
}

//produce un messaggio
void buffer_push(CircularBuffer *cb, Message msg) {
    //attende che ci sia uno slot libero
    sem_wait(&cb->empty);

    //proteggiamo in/out
    pthread_mutex_lock(&cb->mutex);
    cb->buffer[cb->in] = msg;
    cb->in = (cb->in + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&cb->mutex);

    //segnala che c'è un nuovo elemento
    sem_post(&cb->full);
}

//consuma un messaggio
Message buffer_pop(CircularBuffer *cb) {
    Message msg;
    //attende che ci sia un elemento disponibile
    sem_wait(&cb->full);

    //proteggiamo in/out
    pthread_mutex_lock(&cb->mutex);
    msg = cb->buffer[cb->out];
    cb->out = (cb->out + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&cb->mutex);

    //segnala che uno slot è diventato libero
    sem_post(&cb->empty);

    return msg;
}
//conusma un messaggio in modo non bloccante
bool buffer_try_pop(CircularBuffer *cb, Message *msg) {
    // prova il semaforo full senza bloccare
    if (sem_trywait(&cb->full) != 0) {
        return false;
    }
    //c’è almeno un elemento, prendo il mutex e leggo
    pthread_mutex_lock(&cb->mutex);
    *msg = cb->buffer[cb->out];
    cb->out = (cb->out + 1) % BUFFER_SIZE;
    pthread_mutex_unlock(&cb->mutex);
    //libero uno slot empty
    sem_post(&cb->empty);
    return true;
}