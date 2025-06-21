#include "game.h"
#include "spawner.h"
#include "crocodile.h" 
#include "consumer.h"


void create_spawners(CircularBuffer *cb,RiverLane lanes[],pthread_t spawner_tids[],int n_lanes){
    for (int i = 0; i < n_lanes; i++) {
        SpawnerArgs *args = malloc(sizeof *args);
        if (!args) {
            perror("malloc SpawnerArgs");
            exit(EXIT_FAILURE);
        }
        args->cb = cb;
        args->lane = lanes[i];

        if (pthread_create(&spawner_tids[i], NULL,spawner_thread, args) != 0) {
            perror("pthread_create spawner");
            exit(EXIT_FAILURE);
        }
    }
}

void *spawner_thread(void *arg) {
    SpawnerArgs *args = (SpawnerArgs*)arg;
    CircularBuffer *cb = args->cb;
    RiverLane lane = args->lane;
    pthread_t self = pthread_self(); // ottengo il tid del processo corrente
    free(args); 
    bool first = true; //indica il primo spawn
    srand(time(NULL) ^ self); //seme casuale basato sul pid del processo
    //delay iniziale casuale tra 1 e 3 secondi
    int delay_ms = 10 + rand() % 20;
    usleep(delay_ms * 100000);
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
        //se è il primo spawn, il coccodrillo spawna subito
        if (!first) {
            // delay casuale tra 5 e 7 secondi
            int base_sleep = 5 + rand() % 3;
            double total_sleep;
            //regola il tempo di sleep in base alla difficoltà
            if (difficulty == EASY) {
                //rallenta di 1.2×
                total_sleep = base_sleep * 1.2 * 1000000;
            }
            else if (difficulty == HARD) {
                //accelera di 2× (metà delay)
                total_sleep = base_sleep * 1000000 / 2;
            }
            else {
                total_sleep = base_sleep * 1000000;
            }
            //sleep gestito per il toggle pausa
            interruptible_sleep(total_sleep);
        }
        first = false;

        //per ogni spawn lancio un crocodile_thread
        CrocArgs *cargs = malloc(sizeof(*cargs));
        cargs->cb = cb;
        cargs->lane = lane;
        pthread_t tid;
        pthread_create(&tid, NULL, crocodile_thread, cargs);
    }
    return NULL;
}
// funzione sleep che può essere interrotto
void interruptible_sleep(int total_microseconds) {
    int chunk_size = 50000; //50ms 
    int remaining = total_microseconds;
    
    while (remaining > 0) {
        int sleep_time;
        if (remaining > chunk_size) {
            sleep_time = chunk_size;
        } else {
            sleep_time = remaining;
        }
        usleep(sleep_time);
        remaining -= sleep_time;
    }
}