/*
	Author: Tsewei Peng
	Class: CS444 - OS2
	Concurrency homework #3 problem 1: barbershop
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "mt19937ar.c"

#define NUM_CUSTOMERS 5
#define NUM_CHAIRS 3

typedef struct Queue {
	int waiting;
	int vals[NUM_CHAIRS];
} Queue;

typedef struct arg_struct {
	int id;
} arg_struct;

int eax, ebx, ecx, edx;
sem_t barber_lock;
sem_t queue_lock;
sem_t print_lock;
sem_t cut_finish[NUM_CUSTOMERS];
pthread_t customer_threads[NUM_CUSTOMERS];
pthread_t barber_thread;
Queue queue;

void* customer(void* arguments);
void get_hair_cut(int id);
void* barber();
void cut_hair();
void print_queue();
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

	// Initialize Queue;
	int i = 0;
	queue.waiting = 0;
	for (i = 0; i < NUM_CHAIRS; i++) {
		queue.vals[i] = -1;
	}

	// Initialize semaphores
	sem_init(&barber_lock, 0, 0);
	sem_init(&queue_lock, 0, 1);
	sem_init(&print_lock, 0, 1);

	for (i = 0; i < NUM_CUSTOMERS; i++) {
		sem_init(&(cut_finish[i]), 0, 0);
	}

	// Initiate pthreads
	arg_struct* args;

	for (i = 0; i < NUM_CUSTOMERS; i++) {
		args = malloc(sizeof(arg_struct));
		args->id = i;
		pthread_create(&(customer_threads[i]), NULL, &customer, (void*) args);
	}
	pthread_create(&barber_thread, NULL, &barber, NULL);

	// Join threads
	for (i = 0; i < NUM_CUSTOMERS; i++) {
		pthread_join(customer_threads[i], NULL);
	}
	pthread_join(barber_thread, NULL);
	return 0;
}

void* customer(void* arguments) {

	arg_struct* args = (arg_struct*) arguments;
	int id = args->id;
	free(args);
	int next_visit_time = 0;
	int wait = 0;

	while (1) {

		next_visit_time = random_gen(2, 10);

		// If the shop has empty chairs
		sem_wait(&queue_lock);

		sem_wait(&print_lock);
		printf("Customer %d came to the barbershop\n", id);
		sem_post(&print_lock);

		if (queue.waiting < NUM_CHAIRS) {
			// Add customer to queue
			queue.vals[queue.waiting] = id;
			queue.waiting++;

			// Print customer status
			sem_wait(&print_lock);
			printf("==> Customer %d is waiting on a chair\n", id);
			print_queue();
			sem_post(&print_lock);

			// Set wait boolean to 1 so get_hair_cut gets called
			wait = 1;
		}
		else {
			sem_wait(&print_lock);
			printf("Customer %d left the barbershop\n", id);
			sem_post(&print_lock);
		}

		sem_post(&queue_lock);

		// Get hair cut
		if (wait)
			get_hair_cut(id);

		// Sleep and visit next time
		sleep(next_visit_time);
	}
}

void get_hair_cut(int id) {

	// Allow barber to cut, and wait for barber's finish signal.
	sem_post(&barber_lock);
	sem_wait(&(cut_finish[id]));
}

void* barber() {

	int id = 0, i = 0;
	while (1) {

		// Barber wakes to cut hair
		sem_wait(&barber_lock);

		// Cut hair
		cut_hair();

		// Remove customer from the queue
		sem_wait(&queue_lock);

		// Update the queue
		for (i = 0; i < NUM_CHAIRS - 1; i++) {
			queue.vals[i] = queue.vals[i + 1];
		}
		queue.vals[NUM_CHAIRS - 1] = -1;
		queue.waiting--;
		print_queue();

		sem_post(&queue_lock);
	}
}

void cut_hair() {
	int cutTime = random_gen(3, 7);
	int id = queue.vals[0];
	int value = 0;

	// Print barber status
	sem_wait(&print_lock);
	printf("==> Barber cutting hair for customer %d, for %d seconds\n", id, cutTime);
	sem_post(&print_lock);

	// Cut hair, and post signal back to customer
	sleep(cutTime);

	// Print barber status
	sem_wait(&print_lock);
	printf("==> Barber finished cutting hair for customer %d\n", id);
	sem_post(&print_lock);

	// Signal customer that cutting is complete
	sem_post(&(cut_finish[id]));
}

void print_queue() {
	// Function used for printing queue
	int i = 0;
	printf("-----------------------------------------------\n");
	printf("# of Customers In Shop\tCustomer ID in queue\n");
	printf("%d\t\t\t", queue.waiting);

	for (i = 0; i < queue.waiting; i++) {
		printf("%d ", queue.vals[i]);
	}
	printf("\n");
	printf("-----------------------------------------------\n\n");
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
	int i = 0;
	sem_destroy(&barber_lock);
	sem_destroy(&queue_lock);
	sem_destroy(&print_lock);

	for (i = 0; i < NUM_CUSTOMERS; i++) {
		sem_destroy(&(cut_finish[i]));
	}

	kill(0, signal);
	exit(0);
}
