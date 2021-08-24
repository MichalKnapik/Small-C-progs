#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdbool.h>

// turn on to see experiment messages
#define VERBOSE 1

// A safe-ish malloc.
void* smalloc(int tsize) {
  void* n = malloc(tsize);
  if (n == NULL) {
    perror("malloc failed");
    abort();
  }

  return n;
}

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

node* make_node() {
  node* n = smalloc(sizeof(node));
  n->next = NULL;
  pthread_mutex_init(&n->lock, NULL);
  
  return n;
}

tq* make_tq() {
  tq* t = smalloc(sizeof(tq));
  t->head = make_node();

  return t;
}

void free_node(node* nd) {
  if (nd == NULL) return;
  pthread_mutex_destroy(&nd->lock);    
  free(nd);  
}

void free_tq(tq* t) {

  // not thread-safe, run after joining all threads that
  // operate on the list

  node* curr = t->head;
  node* next = NULL;
    
  while (curr != NULL) {
    next = curr->next;
    free_node(curr);
    curr = next;
  }

  free(t);
}

// The hand-over-hand helper. It runs through the nodes of tr locking the successive pairs.
// When it returns, the pointers *prev and *curr are locked (unless curr is NULL)
// and such that *curr is the first node of tr s.t. (*curr)->item.key >= key
// or *curr is NULL if all the keys in tr are smaller than the argument key.
// Call this with prev = NULL and curr = NULL and run tq_oth_unlock afterwards!
void tq_oth(tq* tr, int key, node** pprev, node** pcurr) {
      
  // lock head and its successor (if any)
  node* prev = tr->head;
  pthread_mutex_lock(&prev->lock);
  node* curr = prev->next;
  if (curr != NULL) pthread_mutex_lock(&curr->lock);

  // hand-over-hand locking
  while (curr != NULL && curr->item.key < key) {
    pthread_mutex_unlock(&prev->lock);
    prev = curr;
    curr = prev->next;
    if (curr != NULL) pthread_mutex_lock(&curr->lock);
  }

  *pprev = prev;
  *pcurr = curr;
}

void tq_oth_unlock(node* prev, node* curr){
  pthread_mutex_unlock(&prev->lock);
  if (curr != NULL) pthread_mutex_unlock(&curr->lock);  
}

void tq_insert(tq* tr, payload load) {

  // hand-over-hand search for matching place
  node* prev = NULL;
  node* curr = NULL;
  tq_oth(tr, load.key, &prev, &curr);
  
  // insert the new node
  node* nd = make_node();
  nd->item = load;
  prev->next = nd;
  nd->next = curr;

  tq_oth_unlock(prev, curr);
}

bool tq_remove(tq* tr, int key) {

  // hand-over-hand search for matching place  
  node* prev = NULL;
  node* curr = NULL;
  tq_oth(tr, key, &prev, &curr);

  // remove and free the node if the key matches
  if (curr != NULL && curr->item.key == key) {
    prev->next = curr->next;
    tq_oth_unlock(prev, curr);    
    free_node(curr);
    return true;
  }

  tq_oth_unlock(prev, curr);
  return false;
}

void* tq_get(tq* tr, int key) {

  // hand-over-hand search for matching place  
  node* prev = NULL;
  node* curr = NULL;
  tq_oth(tr, key, &prev, &curr);

  // return the payload if the key matches
  if (curr != NULL && curr->item.key == key) {
    void* retval = curr->item.load;
    tq_oth_unlock(prev, curr);    
    return retval;
  }

  tq_oth_unlock(prev, curr);
  return NULL;
}

typedef struct {
  tq* tr;
  int exp_count;
  int val_bound;
} experiment_args;

void* experiment(void *arg) {
  // this is the random experiment function passed to each thread

  experiment_args* ea = (experiment_args*) arg;
  tq* t = ea->tr;
  int exp_count = ea->exp_count;
  int val_bound = ea->val_bound;
  int options = 3;
  int tid = syscall(SYS_gettid);

  for (int i = 0; i < exp_count; ++i) {
    // do some random operations on the passed tq
    int choice = (int) ((double) rand() / (RAND_MAX + 1.0) * options);
    int rload = (int) ((double) rand() / (RAND_MAX + 1.0) * val_bound);
    int rkey = (int) ((double) rand() / (RAND_MAX + 1.0) * val_bound * 2); 
      
    switch (choice) {

    case 0:; // insert a random value (up to val_bound) to the list
      payload random_load = {rload, NULL};
      if (VERBOSE) printf("thread %d: inserting %d\n", tid, rload);
      tq_insert(t, random_load);
      if (VERBOSE) printf("thread %d: done\n", tid);
      break;

    case 1: // remove a random key from the list
      if (VERBOSE) printf("thread %d: removing the node with key %d\n", tid, rkey);
      char* succ = tq_remove(t, rkey)? "success": "key not found";
      if (VERBOSE) printf("thread %d done, %s: %d\n", tid, succ, rkey);
      break;

    case 2: // get a value from the list
      if (VERBOSE) printf("thread %d: fetching the node with key %d\n", tid, rkey);
      tq_get(t, rkey);
      if (VERBOSE) printf("thread %d done\n", tid);
      break;
    }

  }

  return NULL;
}

int main(int argc, char** argv) {

  int seed = 100;
  srand(seed);

  if (argc != 4) {
    printf("Usage: ./lists no_of_threads inserted_vals val_bound\n");
    return 0;
  }
  int threads_no = 0;
  int exp_count = 0;
  int val_bound = 0;
  sscanf(argv[1], "%d", &threads_no);
  sscanf(argv[2], "%d", &exp_count);
  sscanf(argv[3], "%d", &val_bound);  
  
  tq* tr = make_tq();
  experiment_args eargs = {tr, exp_count, val_bound};

  pthread_t* thread_pool = malloc(threads_no*sizeof(pthread_t));

  // init and run threads
  for (int i = 0; i < threads_no; ++i) {
    pthread_create(&thread_pool[i], NULL, experiment, &eargs);
  }

  // close threads
  for (int i = 0; i < threads_no; ++i) {
    pthread_join(thread_pool[i], NULL);
  }

  free_tq(tr);
  free(thread_pool);
  
  return 0;
}
