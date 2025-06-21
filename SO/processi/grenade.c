#include "game.h"
#include "grenade.h"
#include "map.h"

void grenade_process(int fd_write,int start_x, int start_y, int dx) {
    Message msg;
    Entity grenade;
    grenade.type = ENTITY_GRENADE;
    grenade.width = 1;
    grenade.height = 1;
    grenade.x = start_x;
    grenade.y = start_y;
    grenade.dx = dx;
    grenade.speed = 40000;
    grenade.sprite[0][0] = 'O';
    pid_t my_pid = getpid(); // ottengo il pid del processo corrente

    //manda posizione della granata finchè non esce dallo schermo
    while ((grenade.dx > 0 && grenade.x < MAP_WIDTH) ||(grenade.dx < 0 && grenade.x + grenade.width > 0)) {
        msg.type = MSG_GRENADE_UPDATE;
        msg.entity = grenade;
        msg.id = my_pid;
        write(fd_write, &msg, sizeof(msg));
        //sposto la granata in base alla sua direzione
        grenade.x += grenade.dx;
        usleep(grenade.speed);
    }

    //messaggio di despawn
    msg.type = MSG_GRENADE_DESPAWN;
    msg.entity = grenade;
    msg.id = my_pid;
    write(fd_write, &msg, sizeof(msg));


    exit(0);
}

void draw_grenade(Entity *grenade) {
    attron(COLOR_PAIR(map[grenade->y][grenade->x]));
    mvaddch(grenade->y, grenade->x, grenade->sprite[0][0]);
    attroff(COLOR_PAIR(map[grenade->y][grenade->x]));
}

void clear_grenade(Entity *grenade) {
    for (int i = 0; i < grenade->height; i++) {
        for (int j = 0; j < grenade->width; j++) {
            attron(COLOR_PAIR(map[grenade->y + i][grenade->x + j]));
            mvaddch(grenade->y + i, grenade->x + j, ' ');
            attroff(COLOR_PAIR(map[grenade->y + i][grenade->x + j]));
        }
    }
}

