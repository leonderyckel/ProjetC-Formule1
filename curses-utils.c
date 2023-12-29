#include <ncurses.h>
#include <stdlib.h>
#include "defs.h"

WINDOW* status_win;
WINDOW* cars_win;
WINDOW* cars_res;
WINDOW* cars_best;
WINDOW* cars_laps;
WINDOW* cars_position;
WINDOW* best_win;
WINDOW* best_win_content;
int num_cars;
int max_laps;

char* intToString(int a){
	char *buffer = malloc(10);
	sprintf(buffer, "%d", a);
	return buffer;
}

void initScreen(int cars){
	status_win = newwin(3, COLS-1, 0, 0);
	cars_win = newwin(cars+4, 11, 3, 0);
	cars_res = newwin(cars+4, COLS-50, 3, 11);
	cars_position = newwin(cars+4, 4, 3, COLS-39);
	cars_laps = newwin(cars+4, 4, 3, COLS-35);
	cars_best = newwin(cars+4, 30, 3, COLS-31);
	best_win = newwin(LINES-cars-7, COLS-1, cars+7, 0);
	best_win_content = newwin(LINES-cars-9, COLS-3, cars+8, 1);
	scrollok(best_win_content, 1);
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	bkgd(COLOR_PAIR(1));
	wbkgd(status_win, COLOR_PAIR(1));
	wbkgd(cars_win, COLOR_PAIR(1));
	wbkgd(cars_res, COLOR_PAIR(1));
	wbkgd(cars_laps, COLOR_PAIR(1));
	wbkgd(cars_best, COLOR_PAIR(1));
	wbkgd(best_win, COLOR_PAIR(1));
	wbkgd(best_win_content, COLOR_PAIR(1));
	wbkgd(cars_position, COLOR_PAIR(1));

	init_pair(14, COLOR_RED, COLOR_WHITE);
	init_pair(15, COLOR_GREEN, COLOR_WHITE);
	num_cars = cars;
	max_laps = ((COLS-50)/7);
	mvprintw(COLS-1, 1, intToString(COLS-43));
	mvprintw(COLS-1, 5, intToString(max_laps));
	refresh();
}

void borderView(){
	box(status_win, 0 , 0);
	box(cars_win, 0 , 0);
	box(cars_res, 0, 0);
	box(cars_laps, 0, 0);
	box(cars_best, 0, 0);
	box(best_win, 0, 0);
	box(cars_position, 0, 0);
	wrefresh(status_win);
	wrefresh(cars_win);
	wrefresh(cars_res);
	wrefresh(cars_best);
	wrefresh(best_win);
	refresh();
}

void clearWin(WINDOW* win, int border){
	wclear(win);
	if(border){
		box(win, 0, 0);
	}
	wrefresh(win);
}

void resetWins(){
	clearWin(status_win, 1);
	clearWin(cars_win, 1);
	clearWin(cars_res, 1);
	clearWin(cars_laps, 1);
	clearWin(cars_best, 1);
	clearWin(best_win, 1);
	clearWin(best_win_content, 0);
	clearWin(cars_position, 1);
}

void clearScreen(){
	clearWin(status_win, 0);
	clearWin(cars_win, 0);
	clearWin(cars_res, 0);
	clearWin(cars_laps, 0);
	clearWin(cars_best, 0);
	clearWin(best_win, 0);
	clearWin(best_win_content, 0);
	clearWin(cars_position, 0);
	clear();

}

void showFinalResults(pilote* sorted){
	clearScreen();
	box(stdscr, 0, 0);
	for(int i = 0; i < num_cars; i++){
		attron(A_BOLD);
		mvprintw(i+1, 1, "%2d : ", i+1);
		attroff(A_BOLD);
		mvprintw(i+1, 7, "Car n°%2d with best lap : %5.2f", sorted[i].car_id, sorted[i].scores->bestlaps[6].best_lap);
	}
	refresh();
}

void showResults(rank_item* ranks){
	clearScreen();
	box(stdscr, 0, 0);
	for(int i = 0; i < num_cars; i++){
		attron(A_BOLD);
		mvprintw(i+1, 1, "%2d : ", i+1);
		attroff(A_BOLD);
		mvprintw(i+1, 7, "Car n°%2d with best lap : %5.2f", ranks[i].car->car_id, ranks[i].bestlap);
	}
	refresh();
}

void writeStatus(char* status, int typeRace, int numRace){
	wclear(status_win);
	box(status_win, 0 , 0);
	mvwprintw(status_win, 1, 1, "Status : %s", status);
	mvwprintw(status_win, 1, (COLS/2)-4, "Projet F1");
	mvwprintw(status_win, 1, COLS-12, "Race: %d:%d", typeRace, numRace);
	wrefresh(status_win);
}


void updateCarStatus(pilote* p){
	mvwprintw(cars_win, p->cli_idx+1, 1, "      ");
	if(p->status == driving){
		mvwprintw(cars_win, p->cli_idx+1, 1, "%d (D)", p->car_id);
	}else if(p->status == pitstop){
		mvwprintw(cars_win, p->cli_idx+1, 1, "%d (P)", p->car_id);
	}else if(p->status == out){
		mvwprintw(cars_win, p->cli_idx+1, 1, "%d (O)", p->car_id);
		mvwchgat(cars_res, p->cli_idx+1, 1, -1, A_BOLD, 14, NULL);
		wrefresh(cars_res);
	}else if(p->status == eliminated){
		mvwprintw(cars_win, p->cli_idx+1, 1, "%d (E)", p->car_id);
	}else{
		mvwprintw(cars_win, p->cli_idx+1, 1, "%d (?)", p->car_id);
	}
	wrefresh(cars_win);
}

void updateCarLap(pilote* p, scoreboard* sc, int race){
	if(p->lap_cnt >= max_laps){
		int drawIt = 0;
		lap* lap = sc->races[race];
		while(lap != NULL){
			mvwprintw(cars_res, p->cli_idx+1, (7*((drawIt)-1))+2, "%6.2f ", lap->time_s1 + lap->time_s2 + lap->time_s3);
			drawIt++;
			lap = lap->nextlap;
		}
		wrefresh(cars_res);
		sc->races[race] = sc->races[race]->nextlap;
	}else{
		mvwprintw(cars_res, p->cli_idx+1, (7*(p->lap_cnt-1))+2, "%6.2f ", sc->last_lap[race]->time_s1 + sc->last_lap[race]->time_s2 + sc->last_lap[race]->time_s3);
		wrefresh(cars_res);
	}
	mvwprintw(cars_laps, p->cli_idx+1, 1, "%2d", p->lap_cnt);
	wrefresh(cars_laps);
}



void writeHistory(char* text){
	wprintw(best_win_content, "%s\n", text);
	wrefresh(best_win_content);
}

void setBest(int idx, int bestIdx, bestlap lap){
	char* descriptor;
	float time;
	if(bestIdx == 1){
		descriptor = "S1";
		time = lap.best_s1;
	}
	if(bestIdx == 2){
		descriptor = "S2";
		time = lap.best_s2;
	}
	if(bestIdx == 3){
		descriptor = "S3";
		time = lap.best_s3;
	}
	if(bestIdx == 4){
		descriptor = "lap";
		time = lap.best_lap;
	}
	char buffer[512];
	sprintf(buffer, "Car %d has made a better %s : %5.2f", idx, descriptor, time);
	writeHistory(buffer);
	wrefresh(best_win_content);
}

void updateCarBest(pilote* p, bestlap bestLap, int bestGlobalIdx){
	mvwprintw(cars_best, p->cli_idx+1, 2, "%5.2f %5.2f %5.2f : ", bestLap.best_s1, bestLap.best_s2, bestLap.best_s3);
	mvwprintw(cars_best, p->cli_idx+1, 22, "%5.2f ", bestLap.best_lap);
	if(bestGlobalIdx != 0){
		setBest(p->car_id, bestGlobalIdx, bestLap);
	}
	wrefresh(cars_best);
}

void writePosition(pilote* p){
	mvwprintw(cars_position, p->cli_idx+1, 1, "%2d", p->position+1);
	wrefresh(cars_position);
}



