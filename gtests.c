#define _GNU_SOURCE

#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>
#include "ts_queue_oth.h"

// single thread test parameters
const unsigned int bsize = 100;
int loop_runs = 100;

// multithread test parameters
// all threads should be joined in join_interval secs
unsigned int join_interval = 5; 
int threads_no = 50; // the number of created threads
int exp_count = 100; // the number of inserted vals per thread
int val_bound = 10; // the upper bound on inserted values

typedef struct {
  tq* queue;
} ts_queue_oth_fixture;

// @@@@@@@@ tests and test setups @@@@@@@@

static void tq_fixture_single_thread_set_up(ts_queue_oth_fixture *fixture, gconstpointer user_data) {
  fixture->queue = make_tq();
  tq* queue = fixture->queue;
  
  char* buffer;

  // fill the queue
  // (the head is just a placeholder - its keys are ignored in
  // search and other operations)
  queue->head->item.key = 0;
  queue->head->item.load = "Head.\n";  

  for (int i = 1; i <= loop_runs; ++i) {
    buffer = calloc(bsize, sizeof(char)); 
    snprintf(buffer, bsize, "This is a label %d.\n", i);
    payload load = {i, buffer};
    tq_insert(queue, load);
  }
  
}

static void tq_fixture_set_up(ts_queue_oth_fixture *fixture, gconstpointer user_data) {
  fixture->queue = make_tq();  
}

static void tq_fixture_tear_down(ts_queue_oth_fixture *fixture, gconstpointer user_data) {
  node* fnode = fixture->queue->head->next;

  while (fnode != NULL) {
    if (fnode->item.load != NULL) free(fnode->item.load);
    fnode = fnode->next;
  }    

  free_tq(fixture->queue);
}

// A non-threaded tests for some basic operations on queues.

// 1. Addition.
static void test_tq_basic_search_non_threaded(ts_queue_oth_fixture *fixture, gconstpointer user_data) {

  char* buffer;
  tq* queue = fixture->queue;

  // traverse the queue and check the results (this is only correct when single-threaded)
  node* fnode = queue->head;
  unsigned int cntr = 0;
  char fixed_buffer[bsize];
  buffer = fixed_buffer;

  while (fnode != NULL) {

    const char* msg = (const char*)fnode->item.load;

    g_assert_cmpuint(fnode->item.key, ==, cntr);
    
    if (cntr == 0) g_assert_cmpstr(msg, ==, "Head.\n");
    else {
      snprintf(buffer, bsize, "This is a label %d.\n", cntr);
      g_assert_cmpstr(msg, ==, buffer);      
    }

    ++cntr;
    fnode = fnode->next;
  }

  // traverse the queue by finding keys
  memset(buffer, '\0', bsize);
  for (int i = 1; i <= loop_runs; ++i) {
    const char* msg = ((const char*)tq_get(queue, i));
    snprintf(buffer, bsize, "This is a label %d.\n", i);
    g_assert_cmpstr(msg, ==, buffer);
  }

  // ensure that unknown keys are not in the queue
  for (int i = -10; i <= 10*loop_runs; i = i<0?i+1:i+loop_runs+1) {
    const char* msg = ((const char*)tq_get(queue, i));
    g_assert_cmpstr(msg, ==, NULL);
  }

}

//2. Removal.
static void test_tq_basic_removal_non_threaded(ts_queue_oth_fixture *fixture, gconstpointer user_data) {
  tq* queue = fixture->queue;

  // iterate over all even keys
  for (int i = 2; i <= loop_runs; i += 2) {
    // get and free the value 
    char* msg = ((char*)tq_get(queue, i));
    if (msg != NULL) free(msg);
    // and remove the node
    bool removed = tq_remove(queue, i);
    g_assert_cmpuint(removed, ==, true);
  }

  node* fnode = queue->head->next;

  // check that there are only odd nodes
  while (fnode != NULL) {
    g_assert_true(fnode->item.key%2 != 0);
    fnode = fnode->next;
  }

}

// A primitive timer-based deadlock check.
static void test_tq_check_deadlocks(ts_queue_oth_fixture *fixture, gconstpointer user_data) {
  tq* queue = fixture->queue;

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += join_interval;

  // setup random multithreaded experiment
  experiment_args eargs = {queue, exp_count, val_bound, 0};
  pthread_t* thread_pool = malloc(threads_no*sizeof(pthread_t));

  // init and run threads
  for (int i = 0; i < threads_no; ++i) {
    pthread_create(&thread_pool[i], NULL, experiment, &eargs);
  }

  // close threads
  unsigned int errors = 0;
  for (int i = 0; i < threads_no; ++i) {
    errors += pthread_timedjoin_np(thread_pool[i], NULL, &ts);
  }

  // check if any of the threads timed out - deadlock possible
  g_assert_true(errors == 0);
  
  free(thread_pool);
}

int main(int argc, char** argv) {
  setlocale (LC_ALL, "");
  
  g_test_init(&argc, &argv, NULL);
  
  g_test_add ("/ts/nonthreaded-test-basic-search", ts_queue_oth_fixture, NULL,
              tq_fixture_single_thread_set_up, test_tq_basic_search_non_threaded,
              tq_fixture_tear_down);

  g_test_add ("/ts/nonthreaded-test-basic-removal", ts_queue_oth_fixture, NULL,
              tq_fixture_single_thread_set_up, test_tq_basic_removal_non_threaded,
              tq_fixture_tear_down);

  g_test_add ("/ts/threaded-random-timed-experiment", ts_queue_oth_fixture, NULL,
              tq_fixture_set_up, test_tq_check_deadlocks,
              tq_fixture_tear_down);

  return g_test_run();  
}
