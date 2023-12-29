#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>
#include "defs.h"
#include "func.h"
#include <ncurses.h>
#include <memory.h>
#include "curses-utils.h"
#include "server.h"

pilote* shm_addr;
pilote* sortArray;
int myPipe;
int bestLap;
int bestS1, bestS2, bestS3;
scoreboard* scoreboards;
int cars_cnt;
int finished_race_count = 0;
int* cars;
int* pids;
int race;

/* BEGIN : PREPARE */

void initServer(int shm_id, int pipes[2], int* _cars, int car_cnt, int* pids_list){
	shm_addr = (pilote*) try_sys_call_ptr(shmat(shm_id, NULL, 0), "Shmat failure");
	sortArray = (pilote*) calloc(car_cnt, sizeof(pilote));
	scoreboards = (scoreboard*) calloc(car_cnt, sizeof(scoreboard));
	cars_cnt = car_cnt;
	cars = _cars;
	pids = pids_list;
	critical(initPilotes);
	close(pipes[1]);
	myPipe = pipes[0];
	bestLap = bestS1 = bestS2 = bestS3 = 999;
	prepareScoreboards();
	signal(SIGALRM, sighandler);
	prepareCLI();
}

void initPilotes(){
	for (int i = 0; i < cars_cnt; i++) {
		(&shm_addr[i])->car_id = cars[i];
		(&shm_addr[i])->status = driving;
		(&shm_addr[i])->scores = &scoreboards[i];
		(&shm_addr[i])->cli_idx = i;
	}
}

void prepareScoreboards(){
	for(int sc_index = 0; sc_index < cars_cnt; sc_index++){
		scoreboard* sc = &scoreboards[sc_index];
		//Init races and bestLaps
		for(int race_index = 0; race_index < 7; race_index++){
			sc->car_id = cars[sc_index];
			sc->last_lap[race_index] = (lap*)(malloc(sizeof(lap)));
			sc->races[race_index] = sc->last_lap[race_index];
			sc->bestlaps[race_index] = (bestlap){ .best_s1 = 999, .best_s2 = 999, .best_s3 = 999, .best_lap = 999 };
		}
	}
}

void prepareCLI(){
	initscr();
	cbreak();
	noecho();
	refresh();
	initScreen((int) cars_cnt);
	borderView();
	writeStatus("Starting", 0, 0);
	refresh();
}

void destroyCLI(){
	echo();
	nocbreak();
	endwin();
}

void critical(void (*call)(void)){
	sem_wait(sem);
	(*call)();
	sem_post(sem);
}

/* END : PREPARE */

