/*
	Author: Tsewei Peng
	Class: CS444 - OS2
	Concurrency homework #3 problem 2: linked list mutual exclusion
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "mt19937ar.c"

#define NUM_SEARCHERS 3
#define NUM_INSERTERS 3
#define NUM_DELETERS 3

#define LIST_INIT_LEN 1000
#define VAL_MIN 0
#define VAL_MAX 100

typedef struct Node {
	int val;
	struct Node* next;
} Node;

typedef struct LinkedList {
	int size;
	struct Node* head;
} LinkedList;

typedef struct arg_struct {
	int id;
} arg_struct;

int eax, ebx, ecx, edx;
sem_t search_locks[NUM_SEARCHERS];
sem_t insert_lock;
sem_t delete_lock;
sem_t print_lock;
pthread_t searchers[NUM_SEARCHERS];
pthread_t inserters[NUM_INSERTERS];
pthread_t deleters[NUM_DELETERS];
LinkedList list;

void* searcher(void* arguments);
void* inserter(void* arguments);
void* deleter(void* arguments);
int search_node(int val);
void insert_node(int val, int pos);
int delete_node(int val);
void print_list();
void init_List();
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

	// Initialize list;
	int i = 0;
	init_List();

	// Initialize semaphores and pthreads
	for (i = 0;i < NUM_SEARCHERS; i++) {
		sem_init(&(search_locks[i]), 0, 1);
	}
	sem_init(&insert_lock, 0, 1);
	sem_init(&delete_lock, 0, 1);
	sem_init(&print_lock, 0, 1);

	arg_struct* args;
	for (i = 0; i < NUM_SEARCHERS; i++) {
		args = malloc(sizeof(arg_struct));
		args->id = i;
		pthread_create(&(searchers[i]), NULL, &searcher, (void*) args);
	}
	for (i = 1; i < NUM_INSERTERS; i++) {
		args = malloc(sizeof(arg_struct));
		args->id = i;
		pthread_create(&(inserters[i]), NULL, &inserter, (void*) args);
	}
	for (i = 0; i < NUM_DELETERS; i++) {
		args = malloc(sizeof(arg_struct));
		args->id = i;
		pthread_create(&(deleters[i]), NULL, &deleter, (void*) args);
	}

	for (i = 0; i < NUM_SEARCHERS; i++) {
		pthread_join(searchers[i], NULL);
	}
	for (i = 0; i < NUM_INSERTERS; i++) {
		pthread_join(inserters[i], NULL);
	}
	for (i = 0; i < NUM_DELETERS; i++) {
		pthread_join(deleters[i], NULL);
	}
}

void* searcher(void* arguments) {

	arg_struct* args = (arg_struct*) arguments;
	int id = args->id;
	free(args);
	int wait = 0;
	int val = 0;
	int status = 0;

	while(1) {

		if (list.size > 0) {

			sem_wait(&(search_locks[id]));

			// Generate value to search
			val = random_gen(VAL_MIN, VAL_MAX);

			sem_wait(&print_lock);
			printf("Searcher ID: %d Value: %d.\n", id, val);
			sem_post(&print_lock);

			status = search_node(val);

			sem_wait(&print_lock);
			if (status == 1)
				printf("--> Searcher ID %d done. Value %d found. List Size: %d.\n", id, val, list.size);
			else printf("--> Searcher ID %d done. Value %d not found. List Size: %d.\n", id, val, list.size);
			sem_post(&print_lock);

			sem_post(&(search_locks[id]));

			wait = random_gen(2, 7);
			sleep(wait);
		}
	}
}

void* inserter(void* arguments) {

	arg_struct* args = (arg_struct*) arguments;
	int id = args->id;
	free(args);
	int wait = 0;
	int val = 0;
	int pos = 0;
	int status = 0;

	while(1) {

		sem_wait(&insert_lock);

		// Generate random value and position to insert
		val = random_gen(VAL_MIN, VAL_MAX);
		pos = random_gen(0, list.size + 1);

		sem_wait(&print_lock);
		printf("\tInserter ID: %d Value: %d at position %d.\n", id, val, pos);
		sem_post(&print_lock);

		insert_node(val, pos);

		sem_wait(&print_lock);
		printf("\t--> Inserter ID %d done. Value %d inserted. List Size: %d.\n", id, val, list.size);
		sem_post(&print_lock);

		sem_post(&insert_lock);

		// Sleep thread
		wait = random_gen(2, 7);
		sleep(wait);
	}
}

void* deleter(void* arguments) {

	arg_struct* args = (arg_struct*) arguments;
	int id = args->id;
	free(args);
	int wait = 0;
	int val = 0;
	int status = 0;
	int i = 0;

	while(1) {

		if (list.size > 0) {

			// Wait for all searchers and inserters to complete
			for (i = 0; i < NUM_SEARCHERS; i++) {
				sem_wait(&(search_locks[i]));
			}
			sem_wait(&insert_lock);
			sem_wait(&delete_lock);

			// Generate value to delete
			val = random_gen(VAL_MIN, VAL_MAX);

			sem_wait(&print_lock);
			printf("\t\tDeleter ID: %d. Value: %d.\n", id, val);
			sem_post(&print_lock);

			status = delete_node(val);

			sem_wait(&print_lock);
			if (status == 1)
				printf("\t\t--> Deleter ID: %d done. Value %d deleted. List Size: %d.\n", id, val, list.size);
			else printf("\t\t--> Deleter ID: %d done. Value %d not found. List Size: %d.\n\n", id, val, list.size);
			sem_post(&print_lock);

			// Restore inserters and searchers semaphores
			sem_post(&delete_lock);
			sem_post(&insert_lock);
			for (i = 0; i < NUM_SEARCHERS; i++) {
				sem_post(&(search_locks[i]));
			}

			// Sleep thread
			wait = random_gen(2, 7);
			sleep(wait);
		}
	}
}

/*
	Linkedlist operations
*/

int search_node(int val) {
	// Traverse through the list. If the node found return 1, else return 0
	Node* temp = list.head;

	while (temp != NULL) {

		if (temp->val == val) return 1;

		temp = temp->next;
	}

	return 0;
}

void insert_node(int val, int pos) {

	Node* new_node = malloc(sizeof(Node));
	new_node->val = val;

	if (list.head == NULL) {
		list.head = new_node;
	}

	else if (pos == 0) {
		// If pos is 0, new_node becomes the new head
		new_node->next = list.head;
		list.head = new_node;
	}

	else {
		// Traverse to the index and insert new_node
		Node* temp = list.head;

		while (pos > 1) {
			temp = temp->next;
			pos--;
		}

		new_node->next = temp->next;
		temp->next = new_node;
	}

	list.size++;
}

int delete_node(int val) {
	// Traverse to the index and delete node, if node deleted return 1, else 0
	Node* temp = list.head;
	Node* prev_temp;

	while (temp != NULL) {

		if (temp->val == val) {

			if (temp == list.head) {
				// Delete first node and move head
				list.head = list.head->next;
			}

			else {
				// Delete node
				prev_temp->next = temp->next;
			}

			free(temp);
			list.size--;
			return 1;
		}

		prev_temp = temp;
		temp = temp->next;
	}

	return 0;
}

void print_list() {
	// Debug function to print the list
	int i = 0;
	Node* temp = list.head;

	for (i = 0; i < list.size; i++) {
		printf("%d, ", temp->val);
		temp = temp->next;
	}

	printf("\n");
}

void init_List() {

	printf("\n----> Initializing list with %d nodes\n", LIST_INIT_LEN);

	int i = 0, val = 0, pos = 0;
	list.head = NULL;
	list.size = 0;

	for (i = 0; i < LIST_INIT_LEN; i++) {
		val = random_gen(VAL_MIN, VAL_MAX);
		pos = random_gen(0, list.size + 1);
		insert_node(val, pos);
	}

	printf("----> List initialized with %d nodes\n\n", list.size);
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
	int i = 0;
	for (i = 0; i < NUM_SEARCHERS; i++) {
		sem_destroy(&(search_locks[i]));
	}
	sem_destroy(&insert_lock);
	sem_destroy(&delete_lock);
	sem_destroy(&print_lock);

	Node* temp;
	while (list.head != NULL) {
		temp = list.head;
		list.head = list.head->next;
		free(temp);
	}

    kill(0, signal);
    exit(0);
}
