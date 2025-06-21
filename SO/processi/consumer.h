#ifndef CONSUMER_H
#define CONSUMER_H

#include "game.h"

#define MAX_CROCS_PER_LANE  3
#define MAX_PROJECTILES 20

//struttura per memorizzare lo stato di un coccodrillo in una corsia del fiume 
typedef struct {
    Entity crocs[MAX_CROCS_PER_LANE];
    Entity prev[MAX_CROCS_PER_LANE];
    pid_t pid[MAX_CROCS_PER_LANE];   
    bool active[MAX_CROCS_PER_LANE];
} CrocLaneState;

void consumer(int fd_read,int fd_write,WINDOW *info_win,pid_t spawner_pids[],int n_spawners,RiverLane lanes[],pid_t frog_pid,pid_t timer_pid);
void frog_move(Entity *frog, Entity *frog_prev, int dx, int dy);
bool frog_water_check(Entity *frog, Entity *frog_prev, CrocLaneState lanes_state[], int lane_y[], int *lives, int frog_start_x, int frog_start_y);
void frog_drift_on_croc(Entity *frog, Entity *frog_prev, Entity *croc);
void check_grenade_projectile_collisions(Entity grenades[], Entity gren_prev[], bool gren_active[], pid_t gren_pid[],Entity projectiles[], Entity proj_prev[], bool proj_active[], pid_t proj_pid[]);
bool check_projectile_hits_frog(Entity *p, Entity *f);
void clean();
void reset_frog_position(Entity *frog,Entity *frog_prev,int frog_start_x,int frog_start_y);
void reset_crocs_state(CrocLaneState lanes_state[]);
void reset_grenades_state(bool gren_active[],Entity gren_prev[],int max_gren);
void reset_projectiles_state(bool proj_active[],Entity proj_prev[],int max_proj);
void restart_round(Entity *frog,Entity *frog_prev,int frog_start_x,int frog_start_y,CrocLaneState lanes_state[],RiverLane lanes[], bool gren_active[], bool proj_active[],Entity gren_prev[],Entity proj_prev[]);
void kill_all_spawners(pid_t spawner_pids[], int n);
void kill_all_crocs(CrocLaneState lanes_state[]);
void kill_all_grenades(pid_t gren_pid[], bool gren_active[]);
void kill_all_projectiles(pid_t proj_pid[], bool proj_active[]);
void kill_all_entities(pid_t spawner_pids[], int n_spawners,CrocLaneState lanes_state[],pid_t gren_pid[], bool gren_active[],pid_t proj_pid[], bool proj_active[]);
void pause_producers(pid_t timer_pid,pid_t spawner_pids[], int n_spawners,pid_t gren_pid[], bool gren_active[],pid_t proj_pid[], bool proj_active[]);
void stop_all_spawners(pid_t spawner_pids[], int n_spawners);
void stop_all_grenades(pid_t gren_pid[], bool gren_active[]);
void stop_all_projectiles(pid_t proj_pid[], bool proj_active[]);
void resume_all_spawners(pid_t spawner_pids[], int n_spawners);
void resume_all_grenades(pid_t gren_pid[], bool gren_active[]);
void resume_all_projectiles(pid_t proj_pid[], bool proj_active[]);
void resume_producers(pid_t timer_pid,pid_t spawner_pids[], int n_spawners,pid_t gren_pid[], bool gren_active[],pid_t proj_pid[], bool proj_active[]);

#endif