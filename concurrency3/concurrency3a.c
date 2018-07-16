/*
	Author: Tsewei Peng
	Class: CS444 - OS2
	Concurrency homework #3 [roblem 1: Resource mutual exclusion
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "mt19937ar.c"

#define NUM_PROCESSES 8
#define MAX_SPACES 3

typedef struct arg_struct {
	int id;
} arg_struct;

typedef struct State {
	int ids[MAX_SPACES];
} State;

typedef struct Resource {
	int counter;
	int full;
	sem_t lock;
} Resource;

int eax, ebx, ecx, edx;
sem_t state_change;
pthread_t processes[NUM_PROCESSES];
Resource resource;
State state;

void* req_resource(void* arguments);
void use_resource(int id);
void free_resource();
void print_status();
void print_processes();
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

	// Initialize State
	int i = 0;
	resource.counter = 0;
	resource.full = 0;
	for (i = 0; i < MAX_SPACES; i++) {
		state.ids[i] = -1;
	}

	// Initialize semaphore and pthreads
	sem_init(&(resource.lock), 0, MAX_SPACES);
	sem_init(&state_change, 0, 1);

	arg_struct* args;
	for (i = 0; i < NUM_PROCESSES; i++) {
		args = malloc(sizeof(arg_struct));
		args->id = i;
		pthread_create(&(processes[i]), NULL, &req_resource, (void*) args);
	}

	for (i = 0; i < NUM_PROCESSES; i++) {
		pthread_join(processes[i], NULL);
	}

	return 0;
}

void* req_resource(void* arguments) {
	// Get process id
	arg_struct* args = (arg_struct*) arguments;
	int id = args->id;
	free(args);
	int wait = 0;

	while(1) {

		// Lock on resource
		use_resource(id);

		// Generate random sleep time
		wait = random_gen(1, 10);
		sleep(wait);

		// Unlock the resource lock
		free_resource();
	}
}

void use_resource(int id) {
	// Wait for free resource lock
	sem_wait(&(resource.lock));
	sem_wait(&state_change);

	// If all resource used, stall lock
	state.ids[resource.counter] = id;
	resource.counter++;

	if (resource.counter == MAX_SPACES) {
		resource.full = 1;
	}
	
	print_status();

	sem_post(&state_change);
}

void free_resource() {

	sem_wait(&state_change);

	int i = 0;
	resource.counter--;
	print_status();

	if (!resource.full) {
		// If resource not stalled
		sem_post(&(resource.lock));
	}
	else if (resource.counter == 0) {
		// If resource stalled, wait for all space to be free
		resource.full = 0;
		print_status();
		for (i = 0; i < MAX_SPACES; i++) {
			sem_post(&(resource.lock));
		}
	}

	sem_post(&state_change);
}

void print_status() {

	printf("Resource Status:\tProcess ID using resource:\n");

	if (resource.full) {
		printf("Stalled\t\t\t");
		print_processes();
	}
	else {
		printf("Open\t\t\t");
		print_processes();
	}

}

void print_processes() {
	int i = 0;
	for (i = 0; i < resource.counter; i++) {
		if (i != MAX_SPACES - 1) {
			printf("%d, ", state.ids[i]);
		} else {
			printf("%d", state.ids[i]);
		}
	}
	printf("\n");
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

	if(ecx & 0x40000000){
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
	sem_destroy(&(resource.lock));
	sem_destroy(&state_change);

    kill(0, signal);
    exit(0);
}
