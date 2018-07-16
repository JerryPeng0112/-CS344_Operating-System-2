/*
	Author: Tsewei Peng
	Class: CS444 - OS2
	Concurrency homework #3 problem 2: Cigarette Smokers Problem
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "mt19937ar.c"

#define AGENT_WAIT_MIN 2
#define AGENT_WAIT_MAX 7
#define SMOKE_WAIT_MIN 3
#define SMOKE_WAIT_MAX 5

typedef struct Table {
	int num_paper;
	int num_tobacco;
	int num_match;
} Table;

int eax, ebx, ecx, edx;
sem_t table_mutex;
sem_t paper_sem;
sem_t tobacco_sem;
sem_t match_sem;
pthread_t agent_thread;
pthread_t smoker_paper_thread;
pthread_t smoker_tobacco_thread;
pthread_t smoker_match_thread;
Table table;

void* agent();
void push(int ingredient);
void push_paper();
void push_tobacco();
void push_match();
void* smoker_paper();
void* smoker_tobacco();
void* smoker_match();
void print_table();
int rdrand(int* num);
int random_gen(int min, int max);
void signal_handler(int signal);

int main() {
	// Set up sigaction
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = signal_handler;
	sigaction(SIGINT, &act, NULL);

	// Initiate table
	table.num_paper = 0;
	table.num_tobacco = 0;
	table.num_match = 0;

	// Initialize semaphores
	sem_init(&table_mutex, 0, 1);
	sem_init(&paper_sem, 0, 0);
	sem_init(&tobacco_sem, 0, 0);
	sem_init(&match_sem, 0, 0);

	// Initialize pthreads
	pthread_create(&agent_thread, NULL, &agent, NULL);
	pthread_create(&smoker_paper_thread, NULL, &smoker_paper, NULL);
	pthread_create(&smoker_tobacco_thread, NULL, &smoker_tobacco, NULL);
	pthread_create(&smoker_match_thread, NULL, &smoker_match, NULL);

	// Join threads
	pthread_join(agent_thread, NULL);
	pthread_join(smoker_paper_thread, NULL);
	pthread_join(smoker_tobacco_thread, NULL);
	pthread_join(smoker_match_thread, NULL);
}

void* agent() {

	int ingredient1 = -1;
	int ingredient2 = -1;
	int sleepTime = 0;

	// Constantly producint ingredients
	while (1) {
		// Generate random 2 unique ingredients
		ingredient1 = random_gen(0, 3);
		do {
			ingredient2 = random_gen(0, 3);
		} while(ingredient1 == ingredient2);

		// Push ingredients to pusher
		push(ingredient1);
		push(ingredient2);

		// Random sleep time
		sleepTime = random_gen(AGENT_WAIT_MIN, AGENT_WAIT_MAX);
		sleep(sleepTime);
	}
}

void push(int ingredient) {
	// Push ingredients
	switch(ingredient) {
		case 0:
			printf("Added ingredient: paper\n");
			push_paper();
			break;
		case 1:
			printf("Added ingredient: tobacco\n");
			push_tobacco();
			break;
		case 2:
			printf("Added ingredient: match\n");
			push_match();
			break;
	}
}

void push_paper() {

	// Wait for table mutex
	sem_wait(&table_mutex);

	// If there is tobacco, make smoker_match smoke
	if (table.num_tobacco) {
		table.num_tobacco--;
		printf("--> Added a cigarette for match smoker.\n\n");
		sem_post(&match_sem);
	}
	// If there is match, make smoker_tobacco smoke
	else if (table.num_match) {
		table.num_match--;
		printf("--> Added a cigarette for tobacco smoker.\n\n");
		sem_post(&tobacco_sem);
	}
	// Add paper to table
	else {
		table.num_paper++;
	}

	sem_post(&table_mutex);
}

void push_tobacco() {

	// Wait for table mutex
	sem_wait(&table_mutex);

	// If there is match, make smoker_paper smoke
	if (table.num_match) {
		table.num_match--;
		printf("--> Added cigarette for paper smoker.\n\n");
		sem_post(&paper_sem);
	}
	// If there is paper, make smoker_match smoke
	else if (table.num_paper) {
		table.num_paper--;
		printf("--> Added cigarette for match smoker.\n\n");
		sem_post(&match_sem);
	}
	// Add tobacco to table
	else {
		table.num_tobacco++;
	}

	sem_post(&table_mutex);
}

void push_match() {

	// Wait for table mutex
	sem_wait(&table_mutex);

	// If there is paper, make smoker_tobacco smoke
	if (table.num_paper) {
		table.num_paper--;
		printf("--> Added cigarette for tobacco smoker.\n\n");
		sem_post(&tobacco_sem);
	}
	// If there is tobacco, make smoker_paper smoke
	else if (table.num_tobacco) {
		table.num_tobacco--;
		printf("--> Added cigarette for paper smoker.\n\n");
		sem_post(&paper_sem);
	}
	// Add match to table
	else {
		table.num_match++;
	}

	sem_post(&table_mutex);
}

void* smoker_paper() {

	int cigars_left = 0;
	int smoke_time = 0;

	while (1) {

		sem_wait(&paper_sem);
		// Get number of cigarettes left for this smoker
		sem_getvalue(&paper_sem, &cigars_left);
		smoke_time = random_gen(SMOKE_WAIT_MIN, SMOKE_WAIT_MAX);

		printf("\t--> Paper smoker smoking. Number of cigarettes left: %d\n", cigars_left);
		sleep(smoke_time);
		printf("\t--> Paper smoker finished smoking\n");
	}
}

void* smoker_tobacco() {

	int cigars_left = 0;
	int smoke_time = 0;

	while (1) {

		sem_wait(&tobacco_sem);
		// Get number of cigarettes left for this smoker
		sem_getvalue(&tobacco_sem, &cigars_left);
		smoke_time = random_gen(SMOKE_WAIT_MIN, SMOKE_WAIT_MAX);

		printf("\t--> Tobacco smoker smoking. Number of cigarettes left: %d\n", cigars_left);
		sleep(smoke_time);
		printf("\t--> Tobacco smoker finished smoking\n");
	}
}

void* smoker_match() {

	int cigars_left = 0;
	int smoke_time = 0;

	while (1) {

		sem_wait(&match_sem);
		// Get number of cigarettes left for this smoker
		sem_getvalue(&match_sem, &cigars_left);
		smoke_time = random_gen(SMOKE_WAIT_MIN, SMOKE_WAIT_MAX);

		printf("\t--> Match smoker smoking. Number of cigarettes left: %d\n", cigars_left);
		sleep(smoke_time);
		printf("\t--> Match smoker finished smoking\n");
	}
}

void print_table() {
	printf("Paper: %d, Tobacco: %d, Match: %d\n", table.num_paper, table.num_tobacco, table.num_match);
}

int rdrand(int *num) {
	unsigned char err;
	__asm__ __volatile__("rdrand %0; setc %1" : "=r" (*num), "=qm" (err));
	return (int) err;
}

int random_gen(int min, int max) {

	int num;
	eax = 0x01;

	__asm__ __volatile__("cpuid;" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(eax));

	if (ecx & 0x40000000){
		rdrand(&num);
	}
	else{
		num = (int) genrand_int32();
	}

	num = abs(num);
	num %= (max - min);
	num += min;

	return num;
}

void signal_handler(int signal) {

	// Destroy semaphores
	sem_destroy(&table_mutex);
	sem_destroy(&paper_sem);
	sem_destroy(&tobacco_sem);
	sem_destroy(&match_sem);

	kill(0, signal);
	exit(0);
}
