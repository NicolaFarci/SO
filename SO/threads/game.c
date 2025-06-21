#include "game.h"
#include "frog.h"
#include "consumer.h"
#include "map.h"
#include "timer.h"
#include "crocodile.h"
#include "spawner.h"
#include "buffer.h"
#include "grenade.h"

int game_state;
int score;
pthread_mutex_t render_mutex;
pthread_mutex_t pause_mutex;
pthread_cond_t pause_cond;
pthread_mutex_t game_state_mutex;
bool paused;


bool start_game() {
    srand(time(NULL));
    pthread_mutex_init(&render_mutex,NULL);
    pthread_mutex_init(&pause_mutex,NULL);
    pthread_cond_init(&pause_cond,NULL);
    pthread_mutex_init(&game_state_mutex, NULL);
    game_state = GAME_RUNNING;
    score = INITIAL_SCORE;
    paused = false;

    int game_starty = (LINES - MAP_HEIGHT) / 2;
    int game_startx = (COLS - MAP_WIDTH) / 2;
    WINDOW *info_win = newwin(INFO_HEIGHT, MAP_WIDTH, game_starty+MAP_HEIGHT-1, game_startx);
    box(info_win,0,0);
    //Inizializza il buffer circolare
    CircularBuffer cb;
    buffer_init(&cb);

    //crea thread rana
    pthread_t frog_tid;
    if (pthread_create(&frog_tid, NULL, frog_thread, &cb) != 0) {
        perror("pthread_create frog");
        exit(EXIT_FAILURE);
    }

    //crea thread timer
    pthread_t timer_tid;
    if (pthread_create(&timer_tid, NULL, timer_thread, &cb) != 0) {
        perror("pthread_create timer");
        exit(EXIT_FAILURE);
    }

    //inizializza le corsie del fiume
    RiverLane lanes[NUM_RIVER_LANES];
    init_lanes(lanes);

    //crea thread spawner per ogni corsia
    pthread_t spawner_tids[NUM_RIVER_LANES] = {0};
    SpawnerArgs *sp_args[NUM_RIVER_LANES];
    create_spawners(&cb, lanes, spawner_tids, NUM_RIVER_LANES);

    consumer(&cb, info_win, lanes, spawner_tids, NUM_RIVER_LANES);
    //termina i thread rana e timer
    pthread_cancel(frog_tid);
    pthread_cancel(timer_tid);
    pthread_join(frog_tid, NULL);
    pthread_join(timer_tid, NULL);
    //termina tutti gli spawner
    for (int i = 0; i < NUM_RIVER_LANES; i++) {
        if (spawner_tids[i] != 0) {
            pthread_cancel(spawner_tids[i]);
            pthread_join(spawner_tids[i], NULL);
        }
    }

    //chiude la finestra delle info
    werase(info_win);
    wrefresh(info_win);
    delwin(info_win);
    //distrugge il buffer circolare
    buffer_destroy(&cb);
    //distrugge mutex e cond
    pthread_cond_destroy(&pause_cond);
    pthread_mutex_destroy(&pause_mutex);
    pthread_mutex_destroy(&render_mutex);
    pthread_mutex_destroy(&game_state_mutex);

    //mostra messaggio di fine partita
    clear();
    if (game_state == GAME_WIN) {
        mvprintw(LINES/2 - 1, (COLS - 11)/2, "HAI VINTO!");
    } else {
        mvprintw(LINES/2 - 1, (COLS - 11)/2, "HAI PERSO!");
    }
    mvprintw(LINES/2, (COLS - 24)/2, "Punteggio Finale: %d", score);
    mvprintw(LINES/2 + 1, (COLS - 29)/2, "Premi 'r' per giocare ancora");
    mvprintw(LINES/2 + 2, (COLS - 21)/2, "Premi 'q' per tornare al menu");
    refresh();

    //attendo r o q
    nodelay(stdscr, FALSE);
    int ch;
    do {
        ch = getch();
    } while (ch != 'r' && ch != 'q');

    //resetto stato per la prossima partita

    return (ch == 'r');
}