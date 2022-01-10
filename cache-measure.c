#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_GNU
#include <sys/time.h>
#include <sched.h>

int fix_CPU(int core_id) {
  /* fix CPU core_id for the process */  
  const pid_t pid = getpid();
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset) != 0 ||
      !CPU_ISSET(core_id, &cpuset)) {
    perror("affinity error");
    exit(1);
  }

  printf("Process %d fixed CPU %d.\n", pid, core_id);
  
  return 0;
}

long get_pagesize(void) {

  long rval = sysconf(_SC_PAGESIZE);
  if (rval == -1) {
    perror("sysconf call error");
    exit(-1);
  }

  return rval;
}

unsigned long long measure_CPU_clock_count() {

    unsigned int a, d;
    asm volatile("rdtsc" : "=a" (a), "=d" (d));

    return (((unsigned long long)a) | (((unsigned long long)d) << 32));
}

#ifndef AS_LIB

int main(int argc, char** argv) {

  if(argc < 2) {
    printf("Usage: cache_measure NUMPAGES LOOPS\n");
    return 0;
  }
  fix_CPU(0);

  const int NUMPAGES = atoi(argv[1]);
  const int LOOPS = atoi(argv[2]);  
  const long PAGESIZE = get_pagesize();
  const long int_entries_pp = PAGESIZE/sizeof(int);

  unsigned long long cc_before, cc_after;  

  printf("Measuring (maybe) cache access time with params NUMPAGES=%d, PAGESIZE=%ld and LOOPS=%d.\n", NUMPAGES, PAGESIZE, LOOPS);    

  /* the torture array that fills NUMPAGES pages */
  int* torture_array = (int*) malloc(PAGESIZE * NUMPAGES);

  /* let's touch every page: init */
  cc_before = measure_CPU_clock_count();
  for (int i = 0; i < NUMPAGES * int_entries_pp; i += int_entries_pp) torture_array[i] = 0;
  cc_after = measure_CPU_clock_count();
  printf("Avg clock count per access for torture array init: %lld.\n", (cc_after - cc_before)/NUMPAGES);

  /* let's touch every page: measure cache */  
  unsigned long long ctavg = 0;
  for (int i = 0; i < LOOPS; ++i) {
    cc_before = measure_CPU_clock_count();
    for (int i = 0; i < NUMPAGES * int_entries_pp; i += int_entries_pp) torture_array[i] = i;
    cc_after = measure_CPU_clock_count();
    ctavg = (ctavg * i + (cc_after - cc_before)) / (i + 1);
  }

  printf("Avg clock count per access for torture array: %lld.\n", ctavg/NUMPAGES);

  free(torture_array);
  printf("Process %d done.\n", getpid());

  return 0;
}

#endif
