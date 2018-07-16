/*
	Author: Tsewei Peng
	Class: CS444 - OS2
	Concurrency homework #1
	Resources used:
	1) https://jlmedina123.wordpress.com/2013/05/03/pthreads-with-mutex-and-semaphores/
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "mt19937ar.c"

#define BUFFER_SIZE 32
#define NUM_PRODUCERS 1
#define NUM_CONSUMERS 1
#define PRODUCER_MIN_WAIT = 3
#define PRODUCER_MAX_WAIT = 7
#define CONSUMER_MIN_WAIT = 2
#define CONSUMER_MAX_WAIT = 9

void* producer();
void* consumer();
int produce();
int consume();
void update_semaphores();
int rdrand(int* num);
int random_gen(int min, int max);
void signal_handler(int signal);

typedef struct Item {
	int value;
	int wait;
} Item;
typedef struct Buffer{
	struct Item items[BUFFER_SIZE];
	int index;
} Buffer;

int eax, ebx, ecx, edx;
pthread_mutex_t buffer_mutex;
pthread_t producers[NUM_PRODUCERS];
pthread_t consumers[NUM_CONSUMERS];
sem_t not_full;
sem_t not_empty;
Buffer buffer;

int main() {

	// Set up sigaction
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = signal_handler;
	sigaction(SIGINT, &act, NULL);

	// Initialize mutex and semaphores
	int i;
	buffer.index = -1;
	pthread_mutex_init(&buffer_mutex, NULL);
	sem_init(&not_full, 0, 1);
	sem_init(&not_empty, 0, 0);

	// Initialize pthreads
	for (i = 0; i < NUM_PRODUCERS; i++) {
		pthread_create(&(producers[i]), NULL, &producer, NULL);
	}
	for (i = 0; i < NUM_CONSUMERS; i++) {
		pthread_create(&(consumers[i]), NULL, &consumer, NULL);
	}

	// Terminate pthreads
	for (i = 0; i < NUM_PRODUCERS; i++) {
		pthread_join(producers[i], NULL);
	}
	for (i = 0; i < NUM_CONSUMERS; i++) {
		pthread_join(consumers[i], NULL);
	}

	return 0;
}

void* producer() {
	while(1) {
		int producer_wait;

		// Wait for the buffer to not be full
		sem_wait(&not_full);
		pthread_mutex_lock(&buffer_mutex);

		// Produce and update semaphores
		producer_wait = produce();
		update_semaphores();

		pthread_mutex_unlock(&buffer_mutex);
		sleep(producer_wait);
	}
}

void* consumer() {
	while(1) {
		int consumer_wait;

		// Wait for the buffer to not be empty
		sem_wait(&not_empty);
		pthread_mutex_lock(&buffer_mutex);

		// Consume and update semaphores
		consumer_wait = consume();
		update_semaphores();

		pthread_mutex_unlock(&buffer_mutex);
		sleep(consumer_wait);
	}
}

int produce() {
	// Generate item in buffer, and generate wait time for producer
	int producer_wait, wait, value, index;
	Item* item;

	producer_wait = random_gen(3, 8);
	wait = random_gen(2, 10);
	value = random_gen(0, 10000);

	buffer.index++;
	index = buffer.index;
	item = &(buffer.items[index]);
	item->wait = wait;
	item->value = value;

	printf("Value Produced: %d,  Wait time: %ds, Items in buffer: %d\n", value, producer_wait, buffer.index + 1);;
	return producer_wait;
}

int consume() {
	// Consume an item in buffer, and get wait time for consumer from item in buffer
	int consumer_wait, value, index;
	Item* item;

	index = buffer.index;
	item = &(buffer.items[index]);
	consumer_wait = item->wait;
	value = item->value;

	buffer.index--;

	printf("Value Consumed: %d Wait time: %ds, Items in buffer: %d\n", value, consumer_wait, buffer.index + 1);
	return consumer_wait;
}

void update_semaphores() {
	// Get semaphore value for not_full & not_empty
	int not_full_val;
	int not_empty_val;
	sem_getvalue(&not_full, &not_full_val);
	sem_getvalue(&not_empty, &not_empty_val);

	// Check whether semaphores need to be updated
	if (buffer.index != BUFFER_SIZE - 1 && (int) not_full_val == 0) {
		sem_post(&not_full);
	}
	if (buffer.index != -1 && (int) not_empty_val == 0) {
		sem_post(&not_empty);
	}
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

	// Destroy Mutex
    pthread_mutex_destroy(&buffer_mutex);

	// Destroy semaphores
	sem_destroy(&not_full);
	sem_destroy(&not_empty);

    kill(0, signal);
    exit(0);
}
