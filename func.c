#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define _GNU_SOURCE
#include <signal.h>
#include <poll.h>

#include "defs.h"
#include "server.h"


#include "curses-utils.h"


volatile sig_atomic_t flag_alarm;
volatile sig_atomic_t flag_race_stop;

int try_sys_call_int(int syscall_ret, char* msg_on_fail) {
	if (syscall_ret != -1) return syscall_ret;
	perror(msg_on_fail);
	exit(EXIT_FAILURE);
}


void* try_sys_call_ptr(void* syscall_ret, char* msg_on_fail) {
	if (syscall_ret != NULL) return syscall_ret;
	perror(msg_on_fail);
	exit(EXIT_FAILURE);
}

sem_t* sem;

void init_semaphore(){
	sem_unlink ("pSem");
	sem = sem_open ("pSem", O_CREAT | O_EXCL, 0644, 1);
	// 1 = Only 1 process at a time doing action on shm
}

/**
 * Flush a pipe and returns data availabilty
 * @param pipe the pipe descriptor
 * @return 0 if no data were available, anything else if there was something on the pipe
 */
int flush_pipe(int pipe){
	char buffer[256];

	if (poll(&(struct pollfd){ .fd = pipe, .events = POLLIN }, 1, 0)==1) {
		while(1){
			if (poll(&(struct pollfd){ .fd = pipe, .events = POLLIN }, 1, 0)==1) {
				read(pipe, &buffer, sizeof(buffer));
			}else{
				break;
			}
		}

		return 1;
	}else{

		return 0;
	}


}




void sighandler(int sig) {
	switch (sig) {
		case SIG_RACE_STOP:
			flag_race_stop = 1;
			break;
		case SIGALRM:
			flag_alarm = 1;
			break;
	}
}

float doSector(pilote* p, float time, int pipe){

	p->time = time;

	if(p->status != pitstop){
		if(++(p->sector) > 3){
			p->sector = 0;
			p->lap_cnt++;
		}
	}

	p->has_changed = 1;
	char status[] = "driving";
	write(pipe, status, sizeof(status));

	return p->time;

}

