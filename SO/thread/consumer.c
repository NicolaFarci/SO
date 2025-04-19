#include "consumer.h"
#include "crocodile.h"
#include "buffer.h"
#include "game.h"
#include "frog.h"
#include "map.h"
#include "grenade.h"
#include "list.h"

// Helper per forzare le entità all'interno della mappa
void clamp_entity(Entity *entity) {
    if (entity->x < 1) entity->x = 1;
    if (entity->y < 1) entity->y = 1;
    if (entity->x + entity->width > MAP_WIDTH - 1)
        entity->x = MAP_WIDTH - 1 - entity->width;
    if (entity->y + entity->height > MAP_HEIGHT - 1)
        entity->y = MAP_HEIGHT - 1 - entity->height;
}

void *consumer_thread(void *arg) {
    ConsumerArgs *args = (ConsumerArgs *)arg;
    RiverLane lanes[NUM_RIVER_LANES];
    for(int i = 0; i < NUM_RIVER_LANES; i++){
    	lanes[i]= args->lanes[i];
    }
    CircularBuffer *buffer = args->buffer;
    WINDOW *game_win = args->game_win;
    WINDOW *info_win = args->info_win;
    int i;
    Entity frog;
    frog_init(&frog);
    bool onwater;
    Entity grenade_left, grenade_right;

    // lista di corsie
    List lane_list[NUM_RIVER_LANES];
    for (i = 0; i < NUM_RIVER_LANES; i++) {
        list_init(&lane_list[i]);
    }

    List proiettili_coccodrilli;
    list_init(&proiettili_coccodrilli);

    ListNode* current;
    ListNode* prev;
    bool found;

    Message msg;
    
    int lives = NUM_LIVES;
    int time = ROUND_TIME;
    int hole_index = -1;
    int holes_reached = 0;

    int lane_idx;
    int croc_idx;

    while (lives > 0) {
        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_PAUSED){
            pthread_mutex_unlock(&game_state_mutex); //sblocca subito per evitare che rimanga bloccato
            usleep(100000); // aspetta un po e ricontrolla se è ancora in pausa
            continue;
        }
        pthread_mutex_unlock(&game_state_mutex);

        pthread_mutex_lock(&game_state_mutex);
        if (game_state == GAME_QUITTING || game_state == GAME_WIN){
            pthread_mutex_unlock(&game_state_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&game_state_mutex);

        msg = buffer_pop(buffer);
        switch (msg.type) {
            case MSG_FROG_UPDATE:
                clear_frog(game_win,&frog);
                switch (msg.id){
                    case KEY_UP:
                    //la rana puo andare sempre verso su fin quando non arriva al bordo della tana
                        if(frog.y > HOLE_Y){
                            frog.y -= VERTICAL_JUMP;

                            //se la rana si trova nella sezione acquatica, controlliamo che si trovi sopra un coccodrillo.
                            if(frog.y<=MAP_HEIGHT - 7 && frog.y > ((MAP_HEIGHT-7)-(2*NUM_RIVER_LANES)) ){
                                if(frog.y==MAP_HEIGHT - 7){
                                    lane_idx=0;
                                }else{
                                    lane_idx++;
                                }
                                
                                onwater=true;
                                ListNode* current = lane_list[lane_idx].head;
                                while (current != NULL) {
                                    if(frog.x>=current->data.entity.x && frog.x+1<=current->data.entity.x+CROCODILE_WIDTH){
                                        onwater=false;
                                        break;
                                    }
                                    current = current->next;
                                }

                                if(onwater){
                                    lane_idx=-1;
                                    lives--;
                                    frog.x = (MAP_WIDTH - FROG_WIDTH ) / 2 ;
                                    frog.y = MAP_HEIGHT - FROG_HEIGHT - 1;
                                    time=ROUND_TIME;
                                    onwater=false;
                                    break;
                                }
                            }

                            
                            if (frog.y == HOLE_Y) {
                                // se la rana si trova nella stessa colonna di una tana può entrare
                                if (frog.x == HOLE_X1 || frog.x == HOLE_X2 || frog.x == HOLE_X3 || frog.x == HOLE_X4 || frog.x == HOLE_X5) {
                                    frog.y = HOLE_Y; // Enta nella tana 
                                    int hole_index = check_hole_reached(&frog);
                                    if (!tane[hole_index].occupied&&hole_index!=-1){
                                        hole_update(game_win,hole_index);
                                        holes_reached++;
                                        update_score(time * 100);
                                        time = ROUND_TIME;
                                        reset_round();
                                        frog.x = (MAP_WIDTH - FROG_WIDTH ) / 2 ;
                                        frog.y = MAP_HEIGHT - FROG_HEIGHT - 1;
                                        } 
                                } else{
                                    frog.y += VERTICAL_JUMP; 
                                }
                            }
                        }
                        
                        break;
                    case KEY_DOWN:
                        if(frog.y<MAP_HEIGHT-FROG_HEIGHT-1){
                            frog.y += VERTICAL_JUMP;

                            //se la rana si trova nella sezione acquatica, controlliamo che si trovi sopra un coccodrillo.
                            if(frog.y<=MAP_HEIGHT - 7 && frog.y>=((MAP_HEIGHT-7)-(2*NUM_RIVER_LANES))){
                                lane_idx--;
                                
                                onwater=true;
                                ListNode* current = lane_list[lane_idx].head;
                                while (current != NULL) {
                                    if(frog.x>=current->data.entity.x &&  
                                        frog.x+1<=current->data.entity.x+CROCODILE_WIDTH){
                                        onwater=false;
                                        break;
                                    }
                                    current = current->next;
                                }

                                if(onwater){
                                    lane_idx=-1;
                                    lives--;
                                    frog.x = (MAP_WIDTH - FROG_WIDTH ) / 2 ;
                                    frog.y = MAP_HEIGHT - FROG_HEIGHT - 1;
                                    time=ROUND_TIME;
                                    break;
                                }
                            }

                            }
                        break;
                    case KEY_LEFT:
                        if(frog.x>FROG_WIDTH){
                            frog.x -= 1;
                            //se la rana si trova nella sezione acquatica, controlliamo che si trovi sopra un coccodrillo.
                            if(frog.y<=MAP_HEIGHT - 7 && frog.y>((MAP_HEIGHT-7)-(2*NUM_RIVER_LANES))){
                                onwater=true;
                                ListNode* current = lane_list[lane_idx].head;
                                while (current != NULL) {
                                    if(frog.x>=current->data.entity.x &&  
                                        frog.x+1<=current->data.entity.x+CROCODILE_WIDTH){
                                        onwater=false;
                                        break;
                                    }
                                    current = current->next;
                                }

                                if(onwater){
                                    lane_idx=-1;
                                    lives--;
                                    frog.x = (MAP_WIDTH - FROG_WIDTH ) / 2 ;
                                    frog.y = MAP_HEIGHT - FROG_HEIGHT - 1;
                                    time=ROUND_TIME;
                                    break;
                                }
                            }
                            }
                        break;
                    case KEY_RIGHT:
                        if(frog.x<MAP_WIDTH-FROG_WIDTH-1){
                            frog.x += 1;
                            //se la rana si trova nella sezione acquatica, controlliamo che si trovi sopra un coccodrillo.
                            if(frog.y<=MAP_HEIGHT - 7 && frog.y>((MAP_HEIGHT-7)-(2*NUM_RIVER_LANES))){
                                onwater=true;
                                ListNode* current = lane_list[lane_idx].head;
                                while (current != NULL) {
                                    if(frog.x>=current->data.entity.x &&  
                                        frog.x+1<=current->data.entity.x+CROCODILE_WIDTH){
                                        onwater=false;
                                        break;
                                    }
                                    current = current->next;
                                }

                                if(onwater){
                                    lane_idx=-1;
                                    lives--;
                                    frog.x = (MAP_WIDTH - FROG_WIDTH ) / 2 ;
                                    frog.y = MAP_HEIGHT - FROG_HEIGHT - 1;
                                    time=ROUND_TIME;
                                    break;
                                }
                            }
                        }
                        break;
                }
                draw_frog(game_win, &frog);
                break;
            case MSG_TIMER_TICK:
                time--;
                if (time <= 0) {
                    lives--;
                    clear_frog(game_win,&frog);
                    time = ROUND_TIME;
                    reset_round();
                }
                break;
            case MSG_GRENADE_LEFT:
                clear_grenade(game_win,&grenade_left);
                grenade_left = msg.entity;
                draw_grenade(game_win,&grenade_left);
                break;
            case MSG_GRENADE_RIGHT:
                clear_grenade(game_win,&grenade_right);
                grenade_right = msg.entity;
                draw_grenade(game_win,&grenade_right);
                break;

            case MSG_GRENADE_SPAWN:
                pthread_mutex_lock(&grenade_mutex);
                if (active_grenades == 0) {
                    pthread_t grenade_left_tid, grenade_right_tid;
                    
                    // Create left grenade
                    GrenadeArgs *grenade_args_left = malloc(sizeof(GrenadeArgs));
                    grenade_args_left->buffer = buffer;
                    grenade_args_left->start_x = frog.x - 1;
                    grenade_args_left->start_y = frog.y;
                    grenade_args_left->dx = -1;
                    grenade_args_left->speed = 40000;
                    
                    // Create right grenade
                    GrenadeArgs *grenade_args_right = malloc(sizeof(GrenadeArgs));
                    grenade_args_right->buffer = buffer;
                    grenade_args_right->start_x = frog.x + FROG_WIDTH;
                    grenade_args_right->start_y = frog.y;
                    grenade_args_right->dx = 1;
                    grenade_args_right->speed = 40000;

                    pthread_create(&grenade_left_tid, NULL, grenade_thread, grenade_args_left);
                    pthread_create(&grenade_right_tid, NULL, grenade_thread, grenade_args_right);
                    pthread_detach(grenade_left_tid);
                    pthread_detach(grenade_right_tid);
                }
                pthread_mutex_unlock(&grenade_mutex);
                break;

            case MSG_CROC_PROJECTILE:
                current = proiettili_coccodrilli.head;
                prev = NULL;
                found= false;
                switch (msg.id){
                    //DESPAWN
                    case -1:
                        while(current!=NULL){
                            if(abs(msg.entity.x - current->data.entity.x) <= 1 && msg.entity.y == current->data.entity.y){
                                clear_grenade(game_win,&current->data.entity);
                                
                                if(prev == NULL){
                                	proiettili_coccodrilli.head = current->next;
                                }else{
                                	prev->next=current->next;
                                }
                                if(current == proiettili_coccodrilli.tail){
                                	proiettili_coccodrilli.tail = prev;
                                }
                                ListNode* temp = current;
                                current = current->next;
                                free(temp);
                                proiettili_coccodrilli.count--;
                                found=true;
                                break;
                            }
                            prev=current;
                            current=current->next;
                        }
                        break;
                    //UPDATE
                    case 0:
                        while(current!=NULL){
                            if(abs(msg.entity.x - current->data.entity.x) <= 1 && msg.entity.y == current->data.entity.y){
                                clear_grenade(game_win,&current->data.entity);
                                current->data = msg;
                                draw_grenade(game_win, &current->data.entity);
                                found=true;
                                break;
                            }
                            prev=current;
                            current=current->next;
                            
                        }
                        break;
                    //SPAWN
                    case 1:
                        beep();
                        list_push(&proiettili_coccodrilli, msg);
                        break;
                    default:
                        break;
                }
                break;

            case MSG_CROC_UPDATE:
                // Find the crocodile in the lane list
                current = lane_list[msg.id].head;
                prev = NULL;
                found = false;

                while (current != NULL) {
                    if (abs(msg.entity.x - current->data.entity.x) <= 1) {
                        clear_crocodile(game_win, &current->data.entity);
                        current->data = msg;
                        draw_crocodile(game_win, &current->data.entity);
                        if(current->data.entity.is_badcroc && (rand()%100)< 5 && current->data.entity.has_shot==false){
                            current->data.entity.has_shot=true;
                            pthread_t croc_projectile;
                            GrenadeArgs* croc_proj_args = malloc(sizeof(GrenadeArgs));

                            croc_proj_args->buffer = buffer;
                            croc_proj_args->speed = 40000;
                            croc_proj_args->dx = current->data.entity.dx;
                            croc_proj_args->start_y = current->data.entity.y;
                            if(croc_proj_args->dx == -1){
                                croc_proj_args->start_x = current->data.entity.x;
                            }else{
                                croc_proj_args->start_x = current->data.entity.x + CROCODILE_WIDTH;
                            }

                            pthread_create(&croc_projectile, NULL, crocodile_projectile_thread, croc_proj_args);
                            pthread_detach(croc_projectile);
                        }

                        //se a muoversi e' l'ultimo coccodrillo della lista
                        if (current == lane_list[msg.id].tail){
                            // Increment movement counter for this lane
                            lanes[msg.id].movements++;
                        
                            // Check if we should spawn a new crocodile
                            if (lanes[msg.id].movements >= lanes[msg.id].movements_to_spawn) {
                                lanes[msg.id].movements = 0;
                                // Spawn new crocodile
                                pthread_t croc_tid;
                                CrocodileArgs* croc_args = malloc(sizeof(CrocodileArgs));
                                croc_args->buffer = buffer;
                                croc_args->lane = &lanes[msg.id];
                                pthread_create(&croc_tid, NULL, crocodile_thread, croc_args);
                                pthread_detach(croc_tid);
                            }
                        }
                        
                        if(frog.x>=msg.entity.x && frog.x+FROG_WIDTH<=msg.entity.x+CROCODILE_WIDTH && frog.y==msg.entity.y){
                            clear_frog(game_win,&frog);
                            frog.x += msg.entity.dx;
                            if (frog.x==0 || frog.x+FROG_WIDTH==MAP_WIDTH){
                                lives--;
                                frog.x = (MAP_WIDTH - FROG_WIDTH ) / 2 ;
                                frog.y = MAP_HEIGHT - FROG_HEIGHT - 1;
                                time=ROUND_TIME;
                            }
                            draw_frog(game_win,&frog);
                        }
                        found = true;
                        break;
                    }
                    prev = current;
                    current = current->next;
                }

                // If not found, add as new crocodile
                if (!found) {
                    list_push(&lane_list[msg.id], msg);
                    draw_crocodile(game_win, &msg.entity);
                }
                break;
            case MSG_CROC_DESPAWN:
                // Remove the crocodile from the lane list
                if (lane_list[msg.id].count > 0) {
                    Message removed_croc = list_pop(&lane_list[msg.id]);
                    clear_crocodile(game_win, &removed_croc.entity);
                }
                break;
                
        }
        if (lives <= 0 || holes_reached == NUM_HOLES) {
            pthread_mutex_lock(&game_state_mutex);
            game_state = GAME_WIN;
            pthread_mutex_unlock(&game_state_mutex);
        }
        pthread_mutex_lock(&render_mutex);
        mvwprintw(info_win, 1, 1, "Lives: %-25d Score: %-25d Time:%4d", lives, score, time);
        wrefresh(info_win);
        pthread_mutex_unlock(&render_mutex);
    }
    werase(info_win);
    wrefresh(info_win);
    werase(game_win);
    wrefresh(game_win);

    // Clean up
    for (i = 0; i < NUM_RIVER_LANES; i++) {
        list_free(&lane_list[i]);
    }

    return NULL;
}

void reset_round() {
    pthread_mutex_lock(&reset_mutex);
    round_reset_flag = 1;
    pthread_mutex_unlock(&reset_mutex);
}

void update_score(int points) {
    pthread_mutex_lock(&render_mutex);
    score += points;
    pthread_mutex_unlock(&render_mutex);
}
