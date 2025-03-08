#ifndef CONSUMER_H
#define CONSUMER_H

#include "game.h"

typedef struct {
    CircularBuffer *buffer;
    WINDOW *game_win;
    WINDOW *info_win;
} ConsumerArgs;

void *consumer_thread(void *arg);
void clamp_entity(Entity *entity);
void reset_round();
void update_score(int points);

#endif
