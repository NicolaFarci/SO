#include "game.h"
#include "crocodile.h"
#include "map.h"
#include "buffer.h"

// Inizializza le corsie
void init_lanes(RiverLane lanes[]) {
    //Inizializza la prima corsia
    lanes[0].direction = (rand() % 2 == 0 ? 1 : -1);
    lanes[0].speed = 200000;
    lanes[0].y = MAP_HEIGHT - (BOTTOM_SIDEWALK+FROG_HEIGHT);
    lanes[0].index = 0;

    //Inizializza le altre corsie
    for (int i = 1; i < NUM_RIVER_LANES; i++) {
        //le corsie sono alternate, quindi la direzione e la velocità sono invertite rispetto alla corsia precedente
        lanes[i].direction = -lanes[i-1].direction; 
        lanes[i].speed = lanes[i-1].speed - 10000; //la velocità diminuisce di 10000 per ogni corsia
        lanes[i].y = lanes[i-1].y - FROG_HEIGHT; //la y diminuisce di FROG_HEIGHT per ogni corsia
        lanes[i].index = i;
    }
}

// Inizializza tutte le proprietà di un singolo coccodrillo
void crocodile_init(Entity *crocodile, RiverLane *lane) {
    crocodile->type = ENTITY_CROCODILE;
    crocodile->width = CROCODILE_WIDTH;
    crocodile->height = FROG_HEIGHT;
    crocodile->y = lane->y;
    crocodile->dx = lane->direction;
    crocodile->speed = lane->speed;
    // se la difficoltà è facile, i crocs si aggiornano più lentamente (50ms in più)
    if (difficulty == EASY) crocodile->speed += 50000;
    // se la difficoltà è difficile, i crocs si aggiornano più velocemente
    else if (difficulty == HARD) crocodile->speed -= 50000;

    // inizializza lo sprite del coccodrillo
    strcpy(crocodile->sprite[0], "<BBBBBBB>");
    strcpy(crocodile->sprite[1], "<BBBBBBB>");
}

void *crocodile_thread(void *arg) {
    CrocArgs *args = (CrocArgs*)arg;
    CircularBuffer *cb = args->cb;
    RiverLane lane = args->lane;
    free(arg);
    pthread_t my_tid = pthread_self(); // ottengo il pid del processo corrente
    srand(time(NULL) ^ my_tid); // ottengo il pid del processo corrente

    //inizializzo un nuovo coccodrillo
    Entity croc;
    crocodile_init(&croc, &lane);
    
    // regola la probabilità di warning/sparo
    int shoot_chance;
    if (difficulty == EASY) shoot_chance = 1; //0.1%
    else if (difficulty == HARD) shoot_chance = 50; //5%
    else shoot_chance = 30; //3%

    //spawn iniziale
    if (lane.direction > 0)
        croc.x = 0; //spawn a sinistra
    else
        croc.x = MAP_WIDTH -croc.width; //spawn a destra

    Message msg;

    //messaggio di spawn
    msg.type = MSG_CROC_SPAWN;
    msg.lane_id = lane.index;
    msg.id = my_tid;
    msg.entity = croc;
    //scrivo il messaggio di spawn nella pipe
    buffer_push(cb, msg);

    bool has_shot = false; //indica se il coccodrillo ha già sparato
    bool prefire_warning = false; //indica se il coccodrillo è in fase di pre-fire
    int prefire_timer = 0; //microsecondi da aspettare per sparare
    int step = 50000; //50 ms per ogni ciclo
    
    msg.type = MSG_CROC_UPDATE;
    //finché il coccodrillo è nello schermo
    while ((croc.dx == 1 && croc.x < MAP_WIDTH) || (croc.dx == -1 && croc.x + croc.width > 0)) {
        pthread_mutex_lock(&pause_mutex);
        while (paused) {
            // Qui il produttore si sospende, non produce nulla
            pthread_cond_wait(&pause_cond, &pause_mutex);
        }
        pthread_mutex_unlock(&pause_mutex);
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN){
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);
        //se non ha ancora sparato,valutiamo se far partire il warning
        if (!has_shot && !prefire_warning && (rand() % 1000) < shoot_chance) {
            prefire_warning = true; //attiva il pre-fire warning
            prefire_timer = 400000; //400 ms prima di sparare
            //cambia sprite per il warning (in base alla direzione)
            if (croc.dx > 0) {
                strcpy(croc.sprite[0], "<BBBBBB//");
                strcpy(croc.sprite[1], "<BBBBBB\\\\");
            } else{
                strcpy(croc.sprite[0], "\\\\BBBBBB>");
                strcpy(croc.sprite[1], "//BBBBBB>");
            }
        }

        //se siamo in fase di pre-fire
        if (prefire_warning && prefire_timer > 0) {
            prefire_timer -= step; //decrementa il timer di pre-fire
        }

        //quando il timer è scaduto, spara
        if (prefire_warning && prefire_timer <= 0) {
            //crea un nuovo messaggio per il proiettile
            Message pmsg;
            pmsg.type = MSG_PROJECTILE_SPAWN;
            pmsg.lane_id = lane.index;
            pmsg.id = my_tid;
            // se il coccodrillo sta andando a destra, il proiettile parte a destra del coccodrillo
            if (croc.dx > 0) {
                pmsg.entity.x = croc.x + croc.width + 1;
                // evita di sparare fuori dallo schermo
                if (pmsg.entity.x >= MAP_WIDTH) 
                    pmsg.entity.x = MAP_WIDTH - 1;
            }   
            // se il coccodrillo sta andando a sinistra, il proiettile parte a sinistra del coccodrillo
            else {
                pmsg.entity.x = croc.x - 2;
                // evita di sparare fuori dallo schermo
                if (pmsg.entity.x < 0) 
                    pmsg.entity.x = 0;
            }
            //posizione y del proiettile è la stessa del coccodrillo
            pmsg.entity.y = croc.y;
            pmsg.entity.dx = croc.dx;
            // scrivo il messaggio di spawn del proiettile nella pipe
            buffer_push(cb, pmsg);

            has_shot = true; //indica che il coccodrillo ha già sparato
            prefire_warning = false; //disattiva il pre-fire warning

            //ripristina sprite normale
            strcpy(croc.sprite[0], "<BBBBBBB>");
            strcpy(croc.sprite[1], "<BBBBBBB>");
        }
        //scrivo il messaggio di aggiornamento del coccodrillo nella pipe
        msg.entity = croc;
        buffer_push(cb, msg);
        //sposto il coccodrillo in base alla sua direzione
        croc.x += croc.dx;
        usleep(croc.speed);
    }

    //messaggio di despawn
    msg.type = MSG_CROC_DESPAWN;
    msg.entity = croc;
    buffer_push(cb, msg);


    pthread_exit(NULL);
}

void *projectile_thread(void *arg) {
    ProjectileArgs *pargs = arg;
    CircularBuffer *cb = pargs->cb;
    Message msg;
    Entity proj;
    proj.type = ENTITY_PROJECTILE;
    proj.width = 1;
    proj.height = 1;
    proj.x = pargs->start_x;
    proj.y = pargs->start_y;
    proj.dx = pargs->dx;
    proj.speed = 40000;
    proj.sprite[0][0] = '=';
    pthread_t my_tid = pthread_self(); // ottengo il pid del processo corrente

    //manda posizione della granata finchè non esce dallo schermo
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
        msg.type = MSG_PROJECTILE_UPDATE;
        msg.entity = proj;
        msg.id = my_tid;
        buffer_push(cb, msg);
        //sposto il proiettile in base alla sua direzione
        proj.x += proj.dx;
        //se il proiettile esce dallo schermo, termina il ciclo
        if (proj.x <= 0 || proj.x >= MAP_WIDTH) break;
        usleep(proj.speed);
    }

    //messaggio di despawn
    msg.type = MSG_PROJECTILE_DESPAWN;
    msg.entity = proj;
    msg.id = my_tid;
    buffer_push(cb, msg);

    free(pargs);
    pthread_exit(NULL);
}
//disegna un coccodrillo sullo schermo
void draw_crocodile(Entity *crocodile) {
    for (int i = 0; i < crocodile->height; i++) {
        for (int j = 0; j < crocodile->width; j++) {
            if (crocodile->x + j >=0 && crocodile->x + j < MAP_WIDTH && crocodile->y + i >= 0 && crocodile->y + i < MAP_HEIGHT) {
                attron(COLOR_PAIR(6));// attiva il colore del coccodrillo
                // disegna il carattere nella posizione del coccodrillo
                mvaddch(crocodile->y + i, crocodile->x + j, crocodile->sprite[i][j]);
                attroff(COLOR_PAIR(6));// disattiva il colore del coccodrillo
            }
        }
    }
}
//disegna un proiettile sullo schermo
void draw_projectile(Entity *projectile) {
    //controlla se le coordinate del proiettile sono valide
    if (projectile->x >= 0 && projectile->x < MAP_WIDTH && projectile->y >= 0 && projectile->y < MAP_HEIGHT) {
        attron(COLOR_PAIR(8)); // attiva il colore del proiettile
        // disegna il carattere del proiettile nella sua posizione
        mvaddch(projectile->y, projectile->x, projectile->sprite[0][0]);
        attroff(COLOR_PAIR(8)); // disattiva il colore del proiettile
    }
}
//cancella un proiettile dallo schermo
void clear_projectile(Entity *projectile) {
    if (projectile->x >= 0 && projectile->x < MAP_WIDTH && projectile->y >= 0 && projectile->y < MAP_HEIGHT) {
        attron(COLOR_PAIR(map[projectile->y][projectile->x])); // attiva il colore della cella in cui si trova il proiettile
        mvaddch(projectile->y, projectile->x, ' '); // cancella il carattere nella posizione del proiettile
        attroff(COLOR_PAIR(map[projectile->y][projectile->x]));
    }
}