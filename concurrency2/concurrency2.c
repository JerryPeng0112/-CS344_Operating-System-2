/*
	Author: Tsewei Peng
	Class: CS444 - OS2
	Concurrency homework #2: Dining Philosophers Problem
	Resources used:
	1) https://en.wikipedia.org/wiki/Dining_philosophers_problem
	2) http://adit.io/posts/2013-05-11-The-Dining-Philosophers-Problem-With-Ron-Swanson.html
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>

#define NUM_FORKS 5

typedef struct arg_struct {
    int id;
} arg_struct;

typedef struct State {
	int status[NUM_FORKS]; // 1: thinking 2: waiting 3: eating
	int left_forks[NUM_FORKS];
	int right_forks[NUM_FORKS];
	int times[NUM_FORKS];
	char** names;
} State;

sem_t change_state;
sem_t forks[NUM_FORKS];
pthread_t philo_threads[NUM_FORKS];
State state;

void* philo_routine(void* arguments);
void think(int id);
void get_forks(int id, int left, int right);
void eat(int id);
void put_forks(int id, int left, int right);
void print_status();
void init();
char** init_names();
void init_state();
void signal_handler(int signal);
void dealloc();

int main() {
	srand(time(NULL));
	init();
	return 0;
}

void* philo_routine(void* arguments) {
	// get philosopher data
	arg_struct* args = (arg_struct*) arguments;
	int id = args->id;
	int left = id ? id - 1 : NUM_FORKS - 1;
	int right = id;
	free(args);

	// routine loop
	while(1) {
		think(id);
		get_forks(id, left, right);
		eat(id);
		put_forks(id, left, right);
	}
}

void think(int id) {

	sem_wait(&change_state);

	int thinkTime = rand() % 20 + 1;
	state.times[id] = thinkTime;
	state.status[id] = 1; // Thinking
	print_status();

	sem_post(&change_state);
	sleep(thinkTime);
}

void get_forks(int id, int left, int right) {

	sem_wait(&change_state);

	state.times[id] = 0;
	state.status[id] = 2; // Waiting
	print_status();

	sem_post(&change_state);

	/*
		IMPORTANT:
		This part prevents deadlock: One of the philosopher chooses
		a different side to get fork first, making it impossible to
		have each philosophers to each hold on to one fork and blocking
		each other from accessing the other fork.
	*/

	if (id == 0) {
		// Get left fork fist
		sem_wait(&(forks[right]));

		sem_wait(&change_state);
		state.right_forks[id] = 1;
		print_status();
		sem_post(&change_state);

		// Get right fork second
		sem_wait(&(forks[left]));
		sem_wait(&change_state);
		state.left_forks[id] = 1;

	} else {
		// Get left fork fist
		sem_wait(&(forks[left]));

		sem_wait(&change_state);
		state.left_forks[id] = 1;
		print_status();
		sem_post(&change_state);

		// Get right fork second
		sem_wait(&(forks[right]));
		sem_wait(&change_state);
		state.right_forks[id] = 1;
	}
}

void eat(int id) {

	int eatTime = rand() % 8 + 2;
	state.times[id] = eatTime;
	state.status[id] = 3; // Eating
	print_status();

	sem_post(&change_state);
	sleep(eatTime);
}

void put_forks(int id, int left, int right) {

	sem_wait(&change_state);

	state.status[id] = 1;
	state.left_forks[id] = 0;
	state.right_forks[id] = 0;

	sem_post(&(forks[left]));
	sem_post(&(forks[right]));

	sem_post(&change_state);
}

void print_status() {
	int i = 0, status = 0;
	char left[3];
	char right[3];
	char statusStr[10];

	printf("\033[2J\033[1;1H");
	printf("Name:\t\tFork ID:\tStatus:\n");
	printf("---------------------------------------\n");

	for (i = 0; i < NUM_FORKS; i++) {
		if (state.left_forks[i]) {
			sprintf(left, "%d", (i ? i : NUM_FORKS));
		} else {
			strcpy(left, "");
		}

		if (state.right_forks[i]) {
			sprintf(right, "%d", i + 1);
		} else {
			strcpy(right, "");
		}

		status = state.status[i];
		switch (status) {
			case 1:
				strcpy(statusStr, "Thinking");
				break;
			case 2:
				strcpy(statusStr, "Waiting");
				break;
			case 3:
				strcpy(statusStr, "Eating");
				break;
			default:
				strcpy(statusStr, "");
				break;
		}

		// Structure for pretty printing the status for each philosophers
		if (i == 0 && state.left_forks[i] == 0) {
			printf("%s\t%s\t\t%s\n", state.names[i], right, statusStr);
		} else {
			printf("%s\t%s %s\t\t%s\n", state.names[i], left, right, statusStr);
		}
	}
}

void init() {
	// Set up sigaction
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = signal_handler;
	sigaction(SIGINT, &act, NULL);

	// init semaphores
	int i = 0, status = 0;
	sem_init(&change_state, 0, 1);
	for (i = 0; i < NUM_FORKS; i++) {
		sem_init(&(forks[i]), 0, 1);
	}

	// init philosopher names
	state.names = init_names();

	// init program state
	init_state();

	// init philosophers' threads, with philosopher id as argument
	arg_struct* args;
	for (i = 0; i < NUM_FORKS; i++) {

		args = malloc(sizeof(arg_struct));
		args->id = i;

		pthread_create(&(philo_threads[i]), NULL, &philo_routine, (void*) args);

		if (status != 0) {
			dealloc();
			exit(status);
		}
	}

	// join threads
	for (i = 0; i < NUM_FORKS; i++) {
		pthread_join(philo_threads[i], NULL);
	}
}

char** init_names() {
	int i;
	char** names = malloc(sizeof(char*) * NUM_FORKS);
	for (i = 0; i < NUM_FORKS; i++) {
		names[i] = malloc(sizeof(char) * 14);
	}
    strcpy(names[0], "Aristotle");
    strcpy(names[1], "Immanuel Kant");
    strcpy(names[2], "Thomas Hobbes");
    strcpy(names[3], "Confucius");
    strcpy(names[4], "David Hume");
	return names;
}

void init_state() {
	int i = 0;
	for (i = 0; i < NUM_FORKS; i++) {
		state.left_forks[i] = 0;
		state.right_forks[i] = 0;
		state.status[i] = 1;
		state.times[i] = 0;
	}
}

void signal_handler(int signal) {

	printf("\nProgram exiting...\n");
	// Free dynamically allocated memory
	dealloc();
	// Destroy semaphores
	int i = 0;
	sem_destroy(&change_state);
	for (i = 0; i < NUM_FORKS; i++) {
		sem_destroy(&(forks[i]));
	}
	exit(0);
}

void dealloc() {
	int i = 0;
	for (i = 0; i < NUM_FORKS; i++) {
		free(state.names[i]);
	}
	free(state.names);
}
