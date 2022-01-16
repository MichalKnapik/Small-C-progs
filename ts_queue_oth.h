#pragma once

#include <stdbool.h>

// Put your items as a load under a key in this struct.
typedef struct {
  int key;
  void* load;
} payload;

// This is a node of the thread-safe queue.
struct node {
  payload item;
  struct node* next;
  pthread_mutex_t lock;  
};

// This is the thread-safe queue struct;
typedef struct node node;
typedef struct {
  // note: the head is dummy and used only as a guard
  node* head; 
} tq;

// @@@@@@@@ TS Queue interface @@@@@@@@

// Return a fresh, empty queue.
tq* make_tq();

// Memory cleanup, etc.
void free_tq(tq* t);

// Queue operations.
void tq_insert(tq* tr, payload load);
bool tq_remove(tq* tr, int key); // return true iff a matching key was found in tr
void* tq_get(tq* tr, int key); // return the pointer to the payload under tr's key

// Auxilliary, for tests and experiments.
typedef struct {
  tq* tr;
  int exp_count;
  int val_bound;
  int verbose;
} experiment_args;

void* experiment(void *arg);
