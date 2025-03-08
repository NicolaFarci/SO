#include "game.h"
#include "frog.h"
#include "consumer.h"
#include "timer.h"
#include "buffer.h"
#include "map.h"


int round_reset_flag = 0;
pthread_mutex_t reset_mutex = PTHREAD_MUTEX_INITIALIZER;

int active_grenades = 0;
pthread_mutex_t grenade_mutex = PTHREAD_MUTEX_INITIALIZER;

int game_state = GAME_RUNNING;
pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

int score = INITIAL_SCORE;

void start_game() {
    // Inizializzo la finestra di gioco
    int game_starty = (LINES - MAP_HEIGHT) / 2;
    int game_startx = (COLS - MAP_WIDTH) / 2;

    WINDOW *game_win = newwin(MAP_HEIGHT, MAP_WIDTH, game_starty, game_startx);
    keypad(game_win, TRUE);
    box(game_win, 0, 0);
    init_bckmap();
    init_holes_positions();
    init_map_holes();
    draw_map(game_win);

    WINDOW *info_win = newwin(INFO_HEIGHT, MAP_WIDTH, game_starty + MAP_HEIGHT, game_startx);
    box(info_win, 0, 0);
    wrefresh(info_win);

    // Inizializzo il buffer circolare
    CircularBuffer buffer;
    buffer_init(&buffer);

    // Argomenti per i thread
    ConsumerArgs consumer_args = {&buffer, game_win, info_win};
    FrogArgs frog_args = {&buffer, game_win};
    TimerArgs timer_args = {&buffer};

    pthread_t frog_tid, consumer_tid,timer_tid;

    // Creazione dei thread
    pthread_create(&frog_tid, NULL, frog_thread, &frog_args);
    pthread_create(&timer_tid, NULL, timer_thread, &timer_args);
    pthread_create(&consumer_tid, NULL, consumer_thread, &consumer_args);

    
    pthread_join(consumer_tid, NULL);
    pthread_join(frog_tid, NULL);
    pthread_join(timer_tid, NULL);

    buffer_destroy(&buffer);    

    // Pulizia della finestra di gioco
    werase(info_win);
    wrefresh(info_win);
    delwin(info_win);

    werase(game_win);
    wrefresh(game_win);
    delwin(game_win);

    // Mostra messaggio finale
    if (game_state == GAME_WIN)
        game_state_win();
    else    
        game_over();

}


void game_state_win(){
    int starty = (LINES - MAP_HEIGHT) / 2;
    int startx = (COLS - MAP_WIDTH) / 2;
    
    WINDOW *win_win = newwin(MAP_HEIGHT, MAP_WIDTH, starty, startx);
    box(win_win, 0, 0);
    mvwprintw(win_win, 17, (MAP_WIDTH - 11)/2, "HAI VINTO!");
    mvwprintw(win_win, 19, (MAP_WIDTH - 24)/2, "Punteggio Finale: %d", score);
    mvwprintw(win_win, 20, (MAP_WIDTH - 29)/2, "Premi 'r' per giocare ancora");
    mvwprintw(win_win, 21, (MAP_WIDTH - 21)/2, "Premi 'q' per uscire");
    wrefresh(win_win);
    
    nodelay(stdscr, FALSE);
    int ch;
    // Il gioco può finire o ricominciare solo se si preme 'q' o 'r'
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'r');
    
    werase(win_win);
    wrefresh(win_win);
    delwin(win_win);
    
    if (ch == 'r') {
        restart_game();
    }
}

void game_over(){
    int starty = (LINES - MAP_HEIGHT) / 2;
    int startx = (COLS - MAP_WIDTH) / 2;
    
    WINDOW *game_over_win = newwin(MAP_HEIGHT, MAP_WIDTH, starty, startx);
    box(game_over_win, 0, 0);
    mvwprintw(game_over_win, 17, (MAP_WIDTH - 11)/2, "HAI PERSO!");
    mvwprintw(game_over_win, 19, (MAP_WIDTH - 24)/2, "Punteggio Finale: %d", score);
    mvwprintw(game_over_win, 20, (MAP_WIDTH - 29)/2, "Premi 'r' per giocare ancora");
    mvwprintw(game_over_win, 21, (MAP_WIDTH - 21)/2, "Premi 'q' per uscire");
    wrefresh(game_over_win);
    
    nodelay(stdscr, FALSE);
    int ch;
    // Il gioco può finire o ricominciare solo se si preme 'q' o 'r'
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'r'); 
    
    werase(game_over_win);
    wrefresh(game_over_win);
    delwin(game_over_win);
    
    if (ch == 'r') {
        restart_game();
    }
}

void restart_game() {
    // Resetta le variabili globali
    pthread_mutex_lock(&reset_mutex);
    round_reset_flag = 0;
    pthread_mutex_unlock(&reset_mutex);
    
    pthread_mutex_lock(&grenade_mutex);
    active_grenades = 0;
    pthread_mutex_unlock(&grenade_mutex);
    
    pthread_mutex_lock(&game_state_mutex);
    game_state = GAME_RUNNING;
    pthread_mutex_unlock(&game_state_mutex);
    
    score = INITIAL_SCORE;
    
    // Puslisce lo schermo
    clear();
    refresh();
    
    // Inizia nuova partita
    start_game();
}
