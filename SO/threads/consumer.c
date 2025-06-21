#include "game.h"
#include "consumer.h"
#include "frog.h"
#include "map.h"
#include "timer.h"
#include "crocodile.h"
#include "grenade.h"
#include "spawner.h"

// Funzione consumatore che gestisce la logica del gioco
void consumer(CircularBuffer *cb, WINDOW *info_win, RiverLane lanes[], pthread_t spawner_tids[], int n_spawners){
    init_bckmap(); //inizializza la mappa
    init_holes_positions(); //inizializza le posizioni delle tane
    init_map_holes(); //inizializza le tane
    draw_map(); //disegna la mappa

    Message msg;
    //variabile per la rana
    Entity frog;

    //tempo rimasto e vite della rana
    int time = ROUND_TIME;
    int lives = NUM_LIVES;
    int hole_index = -1;
    int holes_reached = 0;
    
    //stato dei coccodrilli: tid, croc corrente e precedente, attivi/non
    CrocLaneState lanes_state[NUM_RIVER_LANES] = {0};
    CrocLaneState *lane_state = NULL; //serve solo a rendere più leggibile e compatto il codice

    //calcolo le y di ciascuna corsia
    int lane_y[NUM_RIVER_LANES];
    lane_y[0] = MAP_HEIGHT - (BOTTOM_SIDEWALK+FROG_HEIGHT);
    for (int l = 1; l < NUM_RIVER_LANES; l++) {
        lane_y[l] = lane_y[l-1] - FROG_HEIGHT;
    }

    // stato granate: posizioni, tid, attive/non
    Entity grenades[MAX_GRENADES] = {0};
    Entity gren_prev[MAX_GRENADES] = {0};
    pthread_t gren_tid[MAX_GRENADES] = {0};
    bool gren_active[MAX_GRENADES] = {false};
    int active_grenades = 0;

    // stato proiettili: posizioni, tid, attive/non
    Entity projectiles[MAX_PROJECTILES] = {0};
    Entity proj_prev[MAX_PROJECTILES] = {0};
    pthread_t proj_tid[MAX_PROJECTILES] = {0};
    bool proj_active[MAX_PROJECTILES] = {false};
    int proj_count = 0;
    
    // posizione di partenza della rana
    int frog_start_x = (MAP_WIDTH - FROG_WIDTH) / 2;
    int frog_start_y = MAP_HEIGHT - FROG_HEIGHT;

    //inizializzo la rana
    frog_init(&frog);
    Entity frog_prev = frog;

    //variabili per i msg dei croc
    int lane =-1;
    pthread_t id =-1; //usato anche per granate e proiettili
    
    //direzione della granata/proiettile
    int dir = 0;

    bool paused = false; //flag per la pausa

    while (lives > 0 && game_state == GAME_RUNNING) {
        //prendo un messaggio dal buffer
        if (!buffer_try_pop(cb, &msg)) {
            //niente da leggere, salto il ciclo
            continue;
        }
        //toggle pausa
        if (msg.type == MSG_PAUSE) {
            paused = !paused;
            if (paused) {
                //metto in pausa tutti i produttori tranne la rana
                pause_producers();
                //mostro scritta pausa
                mvprintw(LINES/2-1,(COLS-6)/2,"PAUSA");
                mvprintw(LINES/2,  (COLS-23)/2,"Premi P per riprendere");
                refresh();
            } else {    
                //riprende tutti i produttori
                resume_producers();
                //ridisegno
                clear();
                draw_map();
                werase(info_win);
                wrefresh(info_win);
                refresh();
            }
        }
        if (paused) {
            //scarto tutti i messaggi mentre sono in pausa
            continue;
        }
        // GESTIONE MESSAGGI
        // in base al tipo di messaggio, eseguo le operazioni necessarie
        switch (msg.type) {
            //messaggio del timer
            case MSG_TIMER_TICK:
                time--; //decremento il tempo
                // se il tempo è scaduto, resetto il tempo, decremento le vite e resetto il round
                if (time <= 0) {
                    lives--;
                    kill_all_entities(spawner_tids, NUM_RIVER_LANES,lanes_state,gren_tid, gren_active,proj_tid, proj_active);
                    time = ROUND_TIME;
                    restart_round(&frog,&frog_prev,frog_start_x,frog_start_y,lanes_state,lanes,gren_active,proj_active,gren_prev,proj_prev);
                    proj_count = 0; //resetto il conteggio dei proiettili
                    active_grenades = 0; //resetto il conteggio delle granate
                    //creo i nuovi spawner
                    create_spawners(cb, lanes, spawner_tids, NUM_RIVER_LANES);
                }
                break;
            //messaggio dello spostamento della rana
            case MSG_FROG_UPDATE:
                //sposto rana
                frog_move(&frog, &frog_prev,msg.entity.dx,msg.entity.dy);
                
                //controllo caduta in acqua
                bool fell_in_water = frog_water_check(&frog, &frog_prev, lanes_state,lane_y,&lives,frog_start_x,frog_start_y);
                // se è caduta in acqua, resetto il round
                if (fell_in_water) {
                    lives--; // perde una vita
                    //killo tutte le entità
                    kill_all_entities(spawner_tids, NUM_RIVER_LANES,lanes_state,gren_tid, gren_active,proj_tid, proj_active);
                    //resetto gli stati
                    restart_round(&frog, &frog_prev,frog_start_x, frog_start_y,lanes_state, lanes,gren_active, proj_active,grenades, projectiles);
                    proj_count = 0; //resetto il conteggio dei proiettili
                    active_grenades = 0; //resetto il conteggio delle granate
                    //creo i nuovi spawner
                    create_spawners(cb, lanes, spawner_tids, NUM_RIVER_LANES);
                    time = ROUND_TIME;
                    continue;
                }
                //controllo se la rana ha raggiunto una tana
                //se si, salvo l'indice della tana
                hole_index = check_hole_reached(&frog);
                if (hole_index >= 0) {
                    //aggiorno la mappa per indicare che la tana è occupata
                    hole_update(hole_index); 
                    holes_reached++;
                    //se ha raggiunto tutte le tane, vince
                    if (checkHoles()) {
                        game_state = GAME_WIN;
                        sleep(1); //aspetto un secondo prima di uscire
                        return;
                    }
                    //incremento il punteggio in base al tempo rimasto
                    pthread_mutex_lock(&render_mutex);
                    score += time * 100;
                    pthread_mutex_unlock(&render_mutex);
                    
                    //killo tutte le entità
                    kill_all_entities(spawner_tids, NUM_RIVER_LANES,lanes_state,gren_tid, gren_active,proj_tid, proj_active);
                    //resetto gli stati
                    restart_round(&frog, &frog_prev,frog_start_x, frog_start_y,lanes_state, lanes,gren_active, proj_active,grenades, projectiles);
                    proj_count = 0; //resetto il conteggio dei proiettili
                    active_grenades = 0; //resetto il conteggio delle granate
                    //creo i nuovi spawner
                    create_spawners(cb, lanes, spawner_tids, NUM_RIVER_LANES);
                    time = ROUND_TIME;
                    continue;
                }
                //se la rana prova ad entrare in una tana già raggiunta o in una qualsiasi porzione della parte superiore della mappa
                else if (frog.y == HOLE_Y && hole_index == -1) {
                    lives--; //perde una vita
                    //killo tutte le entità
                    kill_all_entities(spawner_tids, NUM_RIVER_LANES,lanes_state,gren_tid, gren_active,proj_tid, proj_active);
                    //resetto gli stati
                    restart_round(&frog, &frog_prev,frog_start_x, frog_start_y,lanes_state, lanes,gren_active, proj_active,grenades, projectiles);
                    proj_count = 0; //resetto il conteggio dei proiettili
                    active_grenades = 0; //resetto il conteggio delle granate
                    //creo i nuovi spawner
                    create_spawners(cb, lanes, spawner_tids, NUM_RIVER_LANES);
                    time = ROUND_TIME;
                }
                break;
            //messaggio di spawn coccodrillo
            //viene spawnato un nuovo coccodrillo in una corsia
            case MSG_CROC_SPAWN:
                //variabili temporanee
                lane = msg.lane_id;
                id = msg.id;
                lane_state = &lanes_state[lane];
                //controllo se ci sono slot liberi nella corsia
                for (int i = 0; i < MAX_CROCS_PER_LANE; i++) {
                    //se lo slot è libero, lo occupo
                    if (!lane_state->active[i] && lane_state->tid[i] == 0) {
                        //aggiorno lo stato del coccodrillo
                        lane_state->crocs[i] = msg.entity;
                        lane_state->prev[i] = msg.entity;
                        lane_state->tid[i] = id;
                        lane_state->active[i] = true;
                        //disegno il coccodrillo
                        draw_crocodile(&lane_state->crocs[i]);
                        break;
                    }
                }
                break;
            //messaggio di spostamento coccodrillo
            case MSG_CROC_UPDATE:
                //variabili temporanee
                lane = msg.lane_id;
                id = msg.id;
                lane_state = &lanes_state[lane];
                //ciclo per trovare il coccodrillo da aggiornare
                for (int i = 0; i < MAX_CROCS_PER_LANE; i++) {
                    //se il coccodrillo è attivo e il suo tid corrisponde a quello del messaggio
                    if (lane_state->active[i] && lane_state->tid[i] == id) {
                        //aggiorna coccodrillo
                        clear_entity(&lane_state->prev[i]);
                        lane_state->crocs[i] = msg.entity;
                        lane_state->prev[i] = lane_state->crocs[i];
                        draw_crocodile(&lane_state->crocs[i]);

                        //drift della rana se è sopra questo coccodrillo
                        frog_drift_on_croc(&frog,&frog_prev,&lane_state->crocs[i]);

                        //controllo caduta in acqua (nel caso in cui il coccodrillo abbia portato la rana fuori dallo schermo)
                        bool fell_in_water = frog_water_check(&frog, &frog_prev, lanes_state,lane_y,&lives,frog_start_x,frog_start_y);
                        //se la rana è caduta in acqua, resetto il round
                        if (fell_in_water) {
                            lives--; //perde una vita
                            //killo tutte le entità
                            kill_all_entities(spawner_tids, NUM_RIVER_LANES,lanes_state,gren_tid, gren_active,proj_tid, proj_active);
                            //resetto gli stati
                            restart_round(&frog, &frog_prev,frog_start_x, frog_start_y,lanes_state, lanes,gren_active, proj_active,grenades, projectiles);
                            proj_count = 0; //resetto il conteggio dei proiettili
                            active_grenades = 0; //resetto il conteggio delle granate
                            //creo i nuovi spawner
                            create_spawners(cb, lanes, spawner_tids, NUM_RIVER_LANES);
                            time = ROUND_TIME;
                            continue;
                        }
                    }
                }
                break;
            //messaggio di despawn coccodrillo
            case MSG_CROC_DESPAWN:
                //variabili temporanee
                lane = msg.lane_id;
                id = msg.id;
                lane_state = &lanes_state[lane];
                //ciclo per trovare il coccodrillo da rimuovere
                for (int i = 0; i < MAX_CROCS_PER_LANE; i++) {
                    //se il coccodrillo è attivo e il suo tid corrisponde a quello del messaggio
                    if (lane_state->active[i] && lane_state->tid[i] == id) {
                        //rimuovo il coccodrillo
                        clear_entity(&lane_state->prev[i]);
                        lane_state->active[i] = false;
                        lane_state->tid[i] = 0; //resetto il tid del coccodrillo
                        break;
                    }
                }
                break;
            //messaggio di spawn granata
            case MSG_GRENADE_SPAWN: {
                // se ci sono già 2 granate attive, non spawnarne altre
                if (gren_active[0] || gren_active[1]) {
                    break;
                }
                //granata SINISTRA -> slot 0
                GrenadeArgs *gargs0 = malloc(sizeof(GrenadeArgs));
                gargs0->cb = cb;
                gargs0->start_x = frog.x;
                gargs0->start_y = frog.y;
                gargs0->dx = -1;
                gargs0->slot = 0;

                pthread_t gtid0;
                pthread_create(&gtid0, NULL, grenade_thread, gargs0);
                pthread_detach(gtid0);
                // registriamo il thread ID come “id” della granata
                gren_active[0] = true;
                gren_tid[0] = gtid0;
                gren_prev[0] = msg.entity;
                grenades[0] = msg.entity;

                // GRANATA DESTRA -> slot 1
                GrenadeArgs *gargs1 = malloc(sizeof(GrenadeArgs));
                gargs1->cb = cb;
                gargs1->start_x = frog.x;
                gargs1->start_y = frog.y;
                gargs1->dx = +1;
                gargs1->slot = 1;

                pthread_t gtid1;
                pthread_create(&gtid1, NULL, grenade_thread, gargs1);
                pthread_detach(gtid1);
                gren_active[1] = true;
                gren_tid[1] = gtid1;
                gren_prev[1] = msg.entity;
                grenades[1] = msg.entity;

                //imposto il numero di granate attive a 2
                active_grenades = MAX_GRENADES;
                break;
            }
            //messagio di spostamento granata
            case MSG_GRENADE_UPDATE:
                id = msg.id; //id della granata presa dal messaggio
                //ciclo per trovare la granata da aggiornare
                for (int i = 0; i < MAX_GRENADES; i++) {
                    //se la granata è attiva e il suo tid corrisponde a quello del messaggio
                    if (gren_active[i] && gren_tid[i] == id) {
                        //aggiorno la granata
                        clear_grenade(&gren_prev[i]);
                        grenades[i] = msg.entity;
                        gren_prev[i] = grenades[i];
                        draw_grenade(&grenades[i]);
                        break;
                    }
                }
                break;
            //messaggio di despawn granata
            case MSG_GRENADE_DESPAWN:
                id = msg.id; //id della granata presa dal messaggio
                //ciclo per trovare la granata da rimuovere
                for (int i = 0; i < MAX_GRENADES; i++) {
                    //se la granata è attiva e il suo tid corrisponde a quello del messaggio
                    if (gren_active[i] && gren_tid[i] == id) {
                        //rimuovo la granata
                        clear_grenade(&gren_prev[i]);
                        gren_active[i] = false;
                        gren_tid[i] = 0;
                        active_grenades--; //decremento il numero di granate attive
                        break;
                    }
                }
                break;
            //messaggio di spawn proiettile
            case MSG_PROJECTILE_SPAWN: {
                //se siamo già al massimo non sparare
                if (proj_count >= MAX_PROJECTILES) break;
                //posizione del coccodrillo
                int cx = msg.entity.x;
                int cy = msg.entity.y;
                dir = msg.entity.dx;

                pthread_t ptid;
                ProjectileArgs *pargs = malloc(sizeof(*pargs));
                pargs->cb = cb;
                pargs->start_x = msg.entity.x;
                pargs->start_y = msg.entity.y;
                pargs->dx = msg.entity.dx;

                pthread_create(&ptid,NULL,projectile_thread,pargs);
                pthread_detach(ptid);
                //padre registra il nuovo proiettile nel primo slot libero
                for (int i = 0; i < MAX_PROJECTILES; i++) {
                    if (!proj_active[i]) {
                        proj_active[i] = true;
                        proj_tid[i] = ptid;
                        proj_prev[i] = msg.entity;
                        projectiles[i] = msg.entity;
                        proj_count++;
                        break;
                    }
                }
                break;
            }
            //messaggio di spostamento proiettile
            case MSG_PROJECTILE_UPDATE: {
                id = msg.id; //id del proiettile preso dal messaggio
                //ciclo per trovare il proiettile da aggiornare
                for (int i = 0; i < MAX_PROJECTILES; i++) {
                    //se il proiettile è attivo e il suo tid corrisponde a quello del messaggio
                    if (proj_active[i] && proj_tid[i] == id) {
                        //aggiorno il proiettile
                        clear_projectile(&proj_prev[i]);
                        projectiles[i] = msg.entity;
                        draw_projectile(&projectiles[i]);
                        proj_prev[i] = projectiles[i];
                        //controllo se la rana viene colpita
                        if (check_projectile_hits_frog(&projectiles[i], &frog)) {
                            lives--; //perde una vita
                            //killo tutte le entità
                            kill_all_entities(spawner_tids, NUM_RIVER_LANES,lanes_state,gren_tid, gren_active,proj_tid, proj_active);
                            //resetto gli stati
                            restart_round(&frog, &frog_prev,frog_start_x, frog_start_y,lanes_state, lanes,gren_active, proj_active,grenades, projectiles);
                            proj_count = 0; //resetto il conteggio dei proiettili
                            active_grenades = 0; //resetto il conteggio delle granate
                            //creo i nuovi spawner
                            create_spawners(cb, lanes, spawner_tids, NUM_RIVER_LANES);
                            time = ROUND_TIME;
                            continue;
                        }
                        break;
                    }
                }
                break;
            }
            //messaggio di despawn proiettile
            case MSG_PROJECTILE_DESPAWN: {
                id = msg.id; //id del proiettile preso dal messaggio
                //ciclo per trovare il proiettile da rimuovere
                for (int i = 0; i < MAX_PROJECTILES; i++) {
                    //se il proiettile è attivo e il suo tid corrisponde a quello del messaggio
                    if (proj_active[i] && proj_tid[i] == id) {
                        //rimuovo il proiettile
                        clear_projectile(&proj_prev[i]);
                        proj_active[i] = false;
                        proj_tid[i] = 0;
                        proj_count--; //decremento il numero di proiettili attivi
                        break;
                    }
                }
                break;
            }
            //messaggio non riconosciuto
            default:
                break;
        }
        //controllo collisioni tra granate e proiettili
        check_grenade_projectile_collisions(grenades, gren_prev, gren_active, gren_tid,projectiles, proj_prev, proj_active, proj_tid);
        //la rana sarà sempre visibile quindi la disegno per ultima
        draw_entity(&frog);
        // Aggiorna info_win
        werase(info_win);
        box(info_win, 0, 0);
        mvwprintw(info_win, 1, 2,"Lives: %-25dScore: %-25dTime: %-3d",lives, score, time);
        wrefresh(info_win);
        //aggiorno lo schermo
        refresh();
    }
}

// Disegna qualsiasi entità sullo schermo
void draw_entity(Entity *entity) {
    for (int i = 0; i < entity->height; i++) {
        for (int j = 0; j < entity->width; j++) {
            if (entity->y + i >= 0 && entity->y + i < MAP_HEIGHT && entity->x + j >= 0 && entity->x + j < MAP_WIDTH) {
                //attiva il colore della cella in cui si trova l'entità
                attron(COLOR_PAIR(map[entity->y + i][entity->x + j]));
                //disegna il carattere nella posizione dell'entità
                mvaddch(entity->y + i, entity->x + j, entity->sprite[i][j]);
                //disattiva il colore della cella
                attroff(COLOR_PAIR(map[entity->y + i][entity->x + j]));
            }
        }
    }
}

// Cancella una qualsiasi entità dallo schermo
void clear_entity(Entity *entity) {
    for (int i = 0; i < entity->height; i++) {
        for (int j = 0; j < entity->width; j++) {
            if (entity->y + i >= 0 && entity->y + i < MAP_HEIGHT && entity->x + j >= 0 && entity->x + j < MAP_WIDTH) {
                //attiva il colore della cella in cui si trova l'entità
                attron(COLOR_PAIR(map[entity->y + i][entity->x + j]));
                //cancella il carattere nella posizione dell'entità
                mvaddch(entity->y + i, entity->x + j, ' ');
                //disattiva il colore della cella
                attroff(COLOR_PAIR(map[entity->y + i][entity->x + j]));
            }
        }
    }
}

// Muove la rana in base alla direzione dx, dy
void frog_move(Entity *frog, Entity *frog_prev, int dx, int dy) {
    clear_entity(frog_prev);
    //sposto la rana
    frog->x += dx;
    frog->y += dy;

    //non può muoversi fuori dallo schermo
    if (frog->x < 0) 
        frog->x = 0;
    if (frog->x + frog->width > MAP_WIDTH)
        frog->x = MAP_WIDTH - frog->width;
    if (frog->y < 0) 
        frog->y = 0;
    if (frog->y + frog->height > MAP_HEIGHT)
        frog->y = MAP_HEIGHT - frog->height;

    *frog_prev = *frog; //aggiorno la posizione precedente della rana
}
// Controlla se la rana è caduta in acqua
bool frog_water_check(Entity *frog, Entity *frog_prev, CrocLaneState lanes_state[], int lane_y[], int *lives, int frog_start_x, int frog_start_y) {
    bool in_river = false; //indica se la rana è in una corsia del fiume
    bool on_croc  = false; //indica se la rana è sopra un coccodrillo
    int  which_lane = -1; //indice della corsia in cui si trova la rana

    //verifico se la rana si trova sulla Y di una corsia
    for (int l = 0; l < NUM_RIVER_LANES; l++) {
        if (frog->y == lane_y[l]) {
            in_river   = true;
            which_lane = l;
            break;
        }
    }
    if (!in_river) return false;//se non è in nessuna corsia, nulla da fare

    //controllo se "on_croc" in quella corsia
    CrocLaneState *lane_state = &lanes_state[which_lane];
    for (int i = 0; i < MAX_CROCS_PER_LANE; i++) {
        if (!lane_state->active[i]) continue; //se il coccodrillo non è attivo, salta
        Entity *croc = &lane_state->crocs[i]; //prendo il coccodrillo corrente
        //coordinate orizzontali di frog e croc (variaibili per chiarezza)
        int frog_left  = frog->x;
        int frog_right = frog->x + frog->width;
        int croc_left  = croc->x;
        int croc_right = croc->x + croc->width;

        //controllo se la rana è sopra il coccodrillo
        if (frog_right > croc_left && frog_left < croc_right) {
            on_croc = true;
            break;
        }
    }

    //se in_river e NON on_croc allora cade in acqua
    if (in_river && !on_croc) return true;

    return false; // se è in una corsia e on_croc, non cade in acqua
}
// La rana "drifta" su un coccodrillo se si trova sulla sua Y e tra le sue coordinate orizzontali
void frog_drift_on_croc(Entity *frog, Entity *frog_prev, Entity *croc) {
    // controlla se la rana è sulla stessa Y del coccodrillo e se si trova tra le sue coordinate orizzontali
    if (frog->y == croc->y && frog->x >= croc->x && frog->x <  croc->x + croc->width) {
        // cancella la posizione precedente della rana
        clear_entity(frog_prev);
        // aggiorna la posizione della rana in base alla direzione del coccodrillo
        frog->x += croc->dx;
        // limita la rana ai bordi dello schermo
        if (frog->x < 0)
            frog->x = 0;
        if (frog->x + frog->width > MAP_WIDTH)
            frog->x = MAP_WIDTH - frog->width;
        // aggiorna la posizione precedente della rana
        *frog_prev = *frog;
    }
}
// Controlla le collisioni tra granate e proiettili
void check_grenade_projectile_collisions(Entity grenades[], Entity gren_prev[], bool gren_active[], pthread_t gren_tid[],Entity projectiles[], Entity proj_prev[], bool proj_active[], pthread_t proj_tid[]){
    for (int i = 0; i < MAX_GRENADES; i++) {
        if (!gren_active[i]) continue; //se la granata non è attiva, salta
        for (int j = 0; j < MAX_PROJECTILES; j++) {
            if (!proj_active[j]) continue; //se il proiettile non è attivo, salta
            //controlla se la granata e il proiettile si trovano nella stessa posizione
            if (grenades[i].x == projectiles[j].x && grenades[i].y == projectiles[j].y){
                //cancello da schermo
                clear_grenade(&gren_prev[i]);
                clear_projectile(&proj_prev[j]);
                //disattivo
                gren_active[i] = false;
                proj_active[j] = false;
            }
        }
    }
}
// Controlla se un proiettile colpisce la rana
bool check_projectile_hits_frog(Entity *p, Entity *f) {
    return (p->x >= f->x && p->x < f->x + f->width &&
            p->y >= f->y && p->y < f->y + f->height);
}

// resetta lo stato dei coccodrilli
void reset_crocs_state(CrocLaneState lanes_state[]) {
    // per ogni corsia, resetto lo stato dei coccodrilli
    for (int l = 0; l < NUM_RIVER_LANES; l++) {
        for (int i = 0; i < MAX_CROCS_PER_LANE; i++) {
            clear_entity(&lanes_state[l].prev[i]);
            lanes_state[l].active[i] = false;
            lanes_state[l].tid[i] = 0;
        }
            
    }
}
// resetta lo stato delle granate
void reset_grenades_state(bool gren_active[],Entity gren_prev[],int max_gren) {
    //ciclo per ogni granata
    for (int i = 0; i < max_gren; i++) {
        //cancello e disattivo lo stato
        clear_grenade(&gren_prev[i]);
        gren_active[i] = false;
    } 
}
// resetta lo stato dei proiettili
void reset_projectiles_state(bool proj_active[],Entity proj_prev[],int max_proj) {
    //ciclo per ogni proiettile
    for (int i = 0; i < max_proj; i++) {
        //cancello e disattivo lo stato
        clear_projectile(&proj_prev[i]);
        proj_active[i] = false;
    }
}
// Resetta la posizione della rana e la sua posizione precedente
void reset_frog_position(Entity *frog,Entity *frog_prev,int frog_start_x,int frog_start_y) {
    // cancello la posizione precedente della rana
    clear_entity(frog_prev);
    // resetto la posizione della rana
    frog->x = frog_start_x;
    frog->y = frog_start_y;

    *frog_prev = *frog; // aggiorno la posizione precedente della rana
}
// Resetta lo stato di gioco per una nuova round
void restart_round(Entity *frog,Entity *frog_prev,int frog_start_x,int frog_start_y,CrocLaneState lanes_state[],RiverLane lanes[], bool gren_active[], bool proj_active[],Entity grenades[],Entity projectiles[]) {
    reset_frog_position(frog,frog_prev,frog_start_x,frog_start_y); // resetto la posizione della rana
    reset_crocs_state(lanes_state); // resetto lo stato dei coccodrilli
    reset_grenades_state(gren_active,grenades,MAX_GRENADES); // resetto lo stato delle granate
    reset_projectiles_state(proj_active,projectiles,MAX_PROJECTILES); // resetto lo stato dei proiettili
    init_lanes(lanes); //reinizializza le corsie
}

void kill_all_spawners(pthread_t spawner_tids[], int n) {
    for (int i = 0; i < n; i++) {
        pthread_t tid = spawner_tids[i];
        if (tid > 0) {
            //termina il thread
            pthread_cancel(spawner_tids[i]);
            pthread_join(spawner_tids[i], NULL);
            spawner_tids[i] = 0;
        }
    }
}
//termina tutti i coccodrilli
void kill_all_crocs(CrocLaneState lanes_state[]) {
    //ciclo per ogni corsia
    for (int l = 0; l < NUM_RIVER_LANES; l++) {
        //ciclo per ogni coccodrillo nella corsia
        for (int i = 0; i < MAX_CROCS_PER_LANE; i++) {
            //se il coccodrillo è attivo e ha un tid valido
            pthread_t tid = lanes_state[l].tid[i];
            if (lanes_state[l].active[i] && tid > 0) {
                //termina il coccodrillo
                pthread_cancel(tid);
                pthread_join(tid, NULL); 
                lanes_state[l].tid[i] = 0; //resetto il tid del coccodrillo
            }
        }
    }
}
//termina tutte le granate
void kill_all_grenades(pthread_t gren_tid[], bool gren_active[]) {
    //ciclo per ogni granata
    for (int i = 0; i < MAX_GRENADES; i++) {
        //se la granata è attiva e ha un tid valido
        if (gren_active[i] && gren_tid[i] > 0) {
            //termina la granata
            pthread_cancel(gren_tid[i]);
            pthread_join(gren_tid[i], NULL);
            gren_tid[i] = 0; //resetto il tid della granata
        }
    }
}
//termina tutti i proiettili
void kill_all_projectiles(pthread_t proj_tid[], bool proj_active[]) {
    //ciclo per ogni proiettile
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        //se il proiettile è attivo e ha un tid valido
        if (proj_active[i] && proj_tid[i] > 0) {
            //termina il proiettile
            pthread_cancel(proj_tid[i]);
            pthread_join(proj_tid[i], NULL);
            proj_tid[i] = 0; //resetto il tid del proiettile
        }
    }
}
//termina tutti gli spawner, coccodrilli, granate e proiettili
void kill_all_entities(pthread_t spawner_tids[],int n_spawners,CrocLaneState lanes_state[],pthread_t gren_tid[], bool gren_active[],pthread_t proj_tid[], bool proj_active[]){
    kill_all_spawners(spawner_tids, n_spawners);
    kill_all_crocs(lanes_state);
    kill_all_grenades(gren_tid, gren_active);
    kill_all_projectiles(proj_tid, proj_active);
}

// PAUSA: ferma timer, spawner, granate e proiettili
void pause_producers() {
    pthread_mutex_lock(&pause_mutex);
    paused = true; //metto in pausa i produttori
    pthread_mutex_unlock(&pause_mutex);
    //segnala a tutti i thread in attesa che la pausa è iniziata
    pthread_cond_broadcast(&pause_cond);
}
// RIPRENDI: riprendi timer, spawner, granate e proiettili
void resume_producers() {
    pthread_mutex_lock(&pause_mutex);
    paused = false; //riprendo i produttori
    pthread_mutex_unlock(&pause_mutex);
    //segnala a tutti i thread in attesa che la pausa è finita
    pthread_cond_broadcast(&pause_cond);
}
