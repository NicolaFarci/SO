#include "game.h"

#define NUM_OPTIONS 3

Difficulty difficulty = NORMAL;

int main() {
    // Inizializzazione di ncurses
    initscr();
    clear();
    noecho();
    curs_set(0);
    cbreak();
    keypad(stdscr, TRUE);

    // Abilita i colori se supportati
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
    }

    // Controllo dimensione minima del terminale
    int min_rows = MAP_HEIGHT + INFO_HEIGHT;
    int min_cols = MAP_WIDTH;
    bool size_confirmed = false;
    while (!size_confirmed) {
        clear();
        mvprintw(LINES/2, (COLS - 35)/2,"Current size: %d rows x %d columns", LINES, COLS);

        if (LINES == min_rows && COLS == min_cols) {
            attron(A_BOLD | COLOR_PAIR(1));
            mvprintw(LINES/2 - 2, (COLS - 26) / 2,"Terminal size is correct!");
            attroff(A_BOLD | COLOR_PAIR(1));
            mvprintw(LINES/2 + 2, (COLS - 44) / 2,"Press ENTER to continue or resize to adjust");
            
            refresh();
            int ch = getch();
            if (ch == '\n') {
                size_confirmed = true;
            }
        } else {
            mvprintw(LINES/2 - 2, (COLS - 28)/2, "Resize terminal before play");
            mvprintw(LINES/2 + 2, (COLS - 43)/2,"Please set at least %d rows and %d columns", min_rows, min_cols);
        }
        refresh();
        usleep(50000);
    }
    clear();

    int selected = 0;
    while (1) {
        const char *menu_options[NUM_OPTIONS] = {"GIOCA","ISTRUZIONI", "ESCI"};
        // Disegna il menu
        clear();
        mvprintw(10, (COLS - strlen("FROGGER RESURRECTION")) / 2,"FROGGER RESURRECTION");
        
        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (i == selected) {
                attron(A_REVERSE | A_BOLD | COLOR_PAIR(1));
                mvprintw(15 + i*2, (COLS - strlen(menu_options[i])) / 2,"%s", menu_options[i]);
                attroff(A_REVERSE | A_BOLD | COLOR_PAIR(1));
            } else {
                mvprintw(15 + i*2, (COLS - strlen(menu_options[i])) / 2,"%s", menu_options[i]);
            }
        }
        refresh();

        int ch = getch();
        switch (ch) {
            case KEY_UP:
                selected = (selected - 1 + NUM_OPTIONS) % NUM_OPTIONS;
                break;
            case KEY_DOWN:
                selected = (selected + 1) % NUM_OPTIONS;
                break;
            case '\n':
                if (selected == 0) {
                    clear();
                    refresh();
                    difficulty = show_difficulty_menu();
                    clear();
                    refresh();
                    bool play_again;
                    do {
                        clear();
                        refresh();
                        play_again = start_game();  
                        //true = l’utente ha premuto ‘r’ per rigiocare
                        //false = l’utente ha premuto ‘q’ per tornare al menu
                    } while (play_again);
                }
                else if (selected == 1) {
                    show_instructions();
                }
                else if (selected == 2) {
                    exit_program();
                }
                break;
        }
    }

    endwin();
    return 0;
}

void show_instructions() {
    clear();
    mvprintw(3, (COLS - 11)/2, "ISTRUZIONI");
    mvprintw(6, (COLS - 32)/2, "1. Muovi la rana con le frecce.");
    mvprintw(9, (COLS - 43)/2, "2. Evita gli ostacoli e raggiungi la tana.");
    mvprintw(12, (COLS - 38)/2, "3. Premi SPAZIO per lanciare granate.");
    mvprintw(15, (COLS - 38)/2, "Premi un tasto per tornare al menu...");
    refresh();
    getch();
    clear();
    refresh();
}

void exit_program() {
    clear();
    refresh();
    endwin();
    exit(0);
}

Difficulty show_difficulty_menu() {
    int sel = 1; //difficoltà predefinita NORMAL
    const char *diff_options[NUM_OPTIONS] = {"FACILE","NORMALE","DIFFICILE"};
    while (1) {
        clear();
        mvprintw(LINES/2 - 2, (COLS - strlen("Seleziona difficolta'"))/2, "Seleziona difficolta'");
        for (int i = 0; i < NUM_OPTIONS; i++) {
            int y = LINES/2 + i*2;
            int x = (COLS - strlen(diff_options[i]))/2;
            if (i == sel) {
                attron(A_REVERSE | A_BOLD);
                mvprintw(y, x, "%s", diff_options[i]);
                attroff(A_REVERSE | A_BOLD);
            } else {
                mvprintw(y, x, "%s", diff_options[i]);
            }
        }
        refresh();
        int ch = getch();
        switch (ch) {
            case KEY_UP: sel = (sel + NUM_OPTIONS - 1) % NUM_OPTIONS; break;
            case KEY_DOWN: sel = (sel + 1) % NUM_OPTIONS; break;
            case '\n': return sel;
        }
    }
}
