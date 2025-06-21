#ifndef CONSUMER_H
#define CONSUMER_H

#include "game.h"
#include "buffer.h"

#define MAX_CROCS_PER_LANE  3
#define MAX_PROJECTILES 20

//struttura per memorizzare lo stato di un coccodrillo in una corsia del fiume 
typedef struct {
    Entity crocs[MAX_CROCS_PER_LANE];
    Entity prev[MAX_CROCS_PER_LANE];
    pthread_t tid[MAX_CROCS_PER_LANE];   
    bool active[MAX_CROCS_PER_LANE];
} CrocLaneState;


void consumer(CircularBuffer *cb, WINDOW *info_win, RiverLane lanes[], pthread_t spawner_tids[], int n_spawners);
void frog_move(Entity *frog, Entity *frog_prev, int dx, int dy);
bool frog_water_check(Entity *frog, Entity *frog_prev, CrocLaneState lanes_state[], int lane_y[], int *lives, int frog_start_x, int frog_start_y);
void frog_drift_on_croc(Entity *frog, Entity *frog_prev, Entity *croc);
void check_grenade_projectile_collisions(Entity grenades[], Entity gren_prev[], bool gren_active[], pthread_t gren_tid[],Entity projectiles[], Entity proj_prev[], bool proj_active[], pthread_t proj_tid[]);
bool check_projectile_hits_frog(Entity *p, Entity *f);
void reset_frog_position(Entity *frog,Entity *frog_prev,int frog_start_x,int frog_start_y);
void reset_crocs_state(CrocLaneState lanes_state[]);
void reset_grenades_state(bool gren_active[],Entity gren_prev[],int max_gren);
void reset_projectiles_state(bool proj_active[],Entity proj_prev[],int max_proj);
void restart_round(Entity *frog,Entity *frog_prev,int frog_start_x,int frog_start_y,CrocLaneState lanes_state[],RiverLane lanes[], bool gren_active[], bool proj_active[],Entity gren_prev[],Entity proj_prev[]);
void kill_all_spawners(pthread_t spawner_tids[], int n);
void kill_all_crocs(CrocLaneState lanes_state[]);
void kill_all_grenades(pthread_t gren_tid[], bool gren_active[]);
void kill_all_projectiles(pthread_t proj_tid[], bool proj_active[]);
void kill_all_entities(pthread_t spawner_tids[], int n_spawners,CrocLaneState lanes_state[],pthread_t gren_tid[], bool gren_active[],pthread_t proj_tid[], bool proj_active[]);
void pause_producers();
void resume_producers();
void clean_threads(pthread_t tids[], bool active[], int n);

#endif