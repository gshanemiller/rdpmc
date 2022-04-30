#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>                                                                                                      
#include <rdpmc.h>
#include <stdlib.h>

void test1(int cpu) {
  rdpmc_initialize(cpu);

  u_int64_t s0 = rdpmc_readCounter(0);
  u_int64_t s1 = rdpmc_readCounter(1);
  u_int64_t s2 = rdpmc_readCounter(2);
  u_int64_t s3 = rdpmc_readCounter(3);

  // No memory accessed: no LLC refs or misses
  for (unsigned i=0; i<10000000; i++);

  u_int64_t e0 = rdpmc_readCounter(0);
  u_int64_t e1 = rdpmc_readCounter(1);
  u_int64_t e2 = rdpmc_readCounter(2);
  u_int64_t e3 = rdpmc_readCounter(3);

  printf("test1: counter 0: unhalted core cycles: %lu\n", e0-s0);
  printf("test1: counter 1: retired instructions: %lu\n", e1-s1);
  printf("test1: counter 2: LLC references      : %lu\n", e2-s2);
  printf("test1: counter 3: LLC misses          : %lu\n", e3-s3);
}

void test2(int cpu) {
  int *ptr = (int*)malloc(sizeof(int)*100000);

  rdpmc_initialize(cpu);

  u_int64_t s0 = rdpmc_readCounter(0);
  u_int64_t s1 = rdpmc_readCounter(1);
  u_int64_t s2 = rdpmc_readCounter(2);
  u_int64_t s3 = rdpmc_readCounter(3);

  // Memory heavily accessed, however, not randomly
  for (unsigned i=0; i<100000; ++i) {
    *(ptr+i) = 0xdeadbeef;
  }

  u_int64_t e0 = rdpmc_readCounter(0);
  u_int64_t e1 = rdpmc_readCounter(1);
  u_int64_t e2 = rdpmc_readCounter(2);
  u_int64_t e3 = rdpmc_readCounter(3);

  printf("test2: counter 0: unhalted core cycles: %lu\n", e0-s0);
  printf("test2: counter 1: retired instructions: %lu\n", e1-s1);
  printf("test2: counter 2: LLC references      : %lu\n", e2-s2);
  printf("test2: counter 3: LLC misses          : %lu\n", e3-s3);

  free(ptr);
  ptr=0;
}

void test3(int cpu) {
  int *ptr = (int*)malloc(sizeof(int)*1000000);

  rdpmc_initialize(cpu);

  u_int64_t s0 = rdpmc_readCounter(0);
  u_int64_t s1 = rdpmc_readCounter(1);
  u_int64_t s2 = rdpmc_readCounter(2);
  u_int64_t s3 = rdpmc_readCounter(3);

  // Memory heavily accessed randomly
  for (unsigned i=0; i<1000000; ++i) {
    long idx = random() % 1000000;
    *(ptr+idx) = 0xdeadbeef;
  }

  u_int64_t e0 = rdpmc_readCounter(0);
  u_int64_t e1 = rdpmc_readCounter(1);
  u_int64_t e2 = rdpmc_readCounter(2);
  u_int64_t e3 = rdpmc_readCounter(3);

  printf("test3: counter 0: unhalted core cycles: %lu\n", e0-s0);
  printf("test3: counter 1: retired instructions: %lu\n", e1-s1);
  printf("test3: counter 2: LLC references      : %lu\n", e2-s2);
  printf("test3: counter 3: LLC misses          : %lu\n", e3-s3);

  free(ptr);
  ptr=0;
}

int main(int argc, char **argv) {
  int cpu = sched_getcpu();

  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);

  if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
      fprintf(stderr, "Error: Could not pin thread to core %d\n", cpu);
      return 1;
  }

  printf("running pinned on CPU %d\n", cpu);

  test1(cpu);
  test2(cpu);
  test3(cpu);

  return 0;
}
