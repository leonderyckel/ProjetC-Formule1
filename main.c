#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/time.h>
#include <ncurses.h>
#include "defs.h"
#include "func.h"
#include "curses-utils.h"
#include "server.h"

int main(int argc, char *argv[]) {
	/* Cars have to be sent through the args of the cli */
	if (argc == 1) {
		printf("Usage: %s car_id [car_id [car_id ...]]\n", argv[0]);
		exit(1);
	}


	init_semaphore();



	/* Init variable */
	int i, loop_count, tmp, race, carToGet, carToGetAccumulator = 0, r;
	size_t cars_cnt;
	int* cars;
	pilote** startFinalRace;
	int pipes[2];
	int shm_key, shm_id;
	pid_t* pids;
	float bestLap, bestS1, bestS2, bestS3;
	int elimination_counter = 0;


	srand((unsigned int) time(NULL));

	/* Read the args to get the cars */
	cars_cnt = (size_t) argc - 1;

	cars = (int*) try_sys_call_ptr(calloc(cars_cnt, sizeof(int)), "Malloc failure");
	startFinalRace = (pilote**) try_sys_call_ptr(calloc(cars_cnt, sizeof(pilote*)), "Malloc failure");
	for (i = 0; i < cars_cnt; i++) {
		cars[i] = (int) strtol(argv[i + 1], (char **)NULL, 10);
	}

	/* Init the pipes */
	try_sys_call_int(pipe(pipes), "Pipe failure");
	for (i = 0; i < cars_cnt; i++) {

	}

	/*	Init the shamed memory with a random key. Fail after 30 try*/
	loop_count = 0;
	do {
		shm_key = rand();
		shm_id = shmget(shm_key, cars_cnt * sizeof(pilote), IPC_CREAT | IPC_EXCL | 0660);
	} while (loop_count++ < 30 && errno == EEXIST);
	try_sys_call_int(shm_id, "Shmget failure");

	/* Fork */
	pids = (pid_t*) try_sys_call_ptr(malloc(cars_cnt * sizeof(pid_t)), "Malloc failure");
	for (i = 0; i < cars_cnt; i++) {
		pids[i] = try_sys_call_int(fork(), "Fork failure");
		if (pids[i] == 0) break;
	}

	/* Child */
	if (i < cars_cnt) {
		srand((unsigned int) (time(NULL) ^ (getpid() << 16)));

		/* Init variable */
		int sig, car_idx, pipe;
		sigset_t sigset;

		/* Save the index in cars and free useless arrays */
		car_idx = i;
		free(cars);
		free(pids);

		/* Attach the shared memory and get our structure */
		pilote* shm_addr = (pilote*) try_sys_call_ptr(shmat(shm_id, NULL, 0), "Shmat failure");
		pilote* myself = &shm_addr[car_idx];

		/* Save one pipe with write acces to the server
		 * close all the other
		 * free the memory segment of pipes */
		pipe = pipes[1];
		close(pipes[0]);
		//free(pipes);


		/* Signals handled by signal syscall */
		signal(SIG_RACE_STOP, sighandler);

		/* Signals handled by sigwait syscall */
		sigemptyset(&sigset);
		sigaddset(&sigset, SIG_RACE_START);
		sigprocmask(SIG_BLOCK, &sigset, NULL);

		/* Loop until the pilote loose */
		while (myself->status != end) {
			/* Wait for the race start */
			if (sigwait(&sigset, &sig) != 0) {
				perror("Sigwait failure");
				exit(EXIT_FAILURE);
			}

			/* Enter main loop, break when SIG_RACE_STOP is fired */
			flag_race_stop = 0;
			while (!flag_race_stop && myself->status != eliminated) {
				//CRITICAL SECTION
				sem_wait(sem);
				// Variables:
				// pilote* myself: pointer to the section of shared memory usable by this pilote
				// int pipe: fd with write access to the server
				// ======= TODO ========
				//


				int intTime = rand()%30 + 13;
				float time = (float)(intTime);

				if(myself->sector == 2){
					if((rand() % 100) < 2){
						myself->status = pitstop;
						myself->has_changed = 1;
					}
				}



				float confirmed_time = doSector(myself, time, pipe);

				if((rand() % 1000) < 3){
					myself->status = out;
					myself->has_changed = 1;
				}


				sem_post(sem);
				if(myself->status == out){
					char status[] = "out";
					write(pipe, status, sizeof(status));
					break;
				}

				usleep((useconds_t) (confirmed_time * 1000000 / 60 * simulation_divider));

				if(myself->status == pitstop) myself->status = driving;

			}
		}

		//printf("Pilote %d left the tournament! \n", myself->car_id);
		exit(EXIT_SUCCESS);


	/* Father--> pas encore finie */
	

}
