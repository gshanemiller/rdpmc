#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>                                                                                                      
#include <rdpmc.h>
#include <stdlib.h>

const int MAX_INTEGERS = 100000000;

void stats(const char *label, int iters, u_int64_t c0, u_int64_t c1, u_int64_t c2, u_int64_t c3) {
  printf("%s: counter 0: iterations run      : %d\n", label, iters);
  printf("%s: counter 0: unhalted core cycles: %lu\n", label, c0);
  printf("%s: counter 1: retired instructions: %lu\n", label, c1);
  printf("%s: counter 1: ret-instns-per-cycle: %4.3lf\n", label, (double)(c1)/(double)(c0));
  printf("%s: counter 2: LLC references      : %lu\n", label, c2);
  printf("%s: counter 3: LLC misses          : %lu\n", label, c3);
  printf("\n");
}

void test0(int cpu) {
  rdpmc_initialize(cpu);

  u_int64_t s0 = rdpmc_readCounter(0);
  u_int64_t s1 = rdpmc_readCounter(1);
  u_int64_t s2 = rdpmc_readCounter(2);
  u_int64_t s3 = rdpmc_readCounter(3);

  // No memory accessed:
  int i=0;
  do {
    ++i;
  } while (__builtin_expect((i<MAX_INTEGERS),1));

  u_int64_t e0 = rdpmc_readCounter(0);
  u_int64_t e1 = rdpmc_readCounter(1);
  u_int64_t e2 = rdpmc_readCounter(2);
  u_int64_t e3 = rdpmc_readCounter(3);

  stats("test0", MAX_INTEGERS, e0-s0, e1-s1, e2-s2, e3-s3);
}

void test1(int cpu) {
  rdpmc_initialize(cpu);

  u_int64_t s0 = rdpmc_readCounter(0);
  u_int64_t s1 = rdpmc_readCounter(1);
  u_int64_t s2 = rdpmc_readCounter(2);
  u_int64_t s3 = rdpmc_readCounter(3);

  // No memory accessed
  for (int i=0; i<MAX_INTEGERS; i++);

  u_int64_t e0 = rdpmc_readCounter(0);
  u_int64_t e1 = rdpmc_readCounter(1);
  u_int64_t e2 = rdpmc_readCounter(2);
  u_int64_t e3 = rdpmc_readCounter(3);

  stats("test1", MAX_INTEGERS, e0-s0, e1-s1, e2-s2, e3-s3);
}

void test2(int cpu, int *ptr) {
  rdpmc_initialize(cpu);

  u_int64_t s0 = rdpmc_readCounter(0);
  u_int64_t s1 = rdpmc_readCounter(1);
  u_int64_t s2 = rdpmc_readCounter(2);
  u_int64_t s3 = rdpmc_readCounter(3);

  // Memory heavily accessed, however, not randomly
  for (int i=0; i<MAX_INTEGERS; ++i) {
    *(ptr+i) = 0xdeadbeef;
  }

  u_int64_t e0 = rdpmc_readCounter(0);
  u_int64_t e1 = rdpmc_readCounter(1);
  u_int64_t e2 = rdpmc_readCounter(2);
  u_int64_t e3 = rdpmc_readCounter(3);

  stats("test2", MAX_INTEGERS, e0-s0, e1-s1, e2-s2, e3-s3);
}

void test3(int cpu, int *ptr) {
  rdpmc_initialize(cpu);

  u_int64_t s0 = rdpmc_readCounter(0);
  u_int64_t s1 = rdpmc_readCounter(1);
  u_int64_t s2 = rdpmc_readCounter(2);
  u_int64_t s3 = rdpmc_readCounter(3);

  // Memory heavily accessed randomly
  for (int i=0; i<MAX_INTEGERS; ++i) {
    long idx = random() % MAX_INTEGERS;
    *(ptr+idx) = 0xdeadbeef;
  }

  u_int64_t e0 = rdpmc_readCounter(0);
  u_int64_t e1 = rdpmc_readCounter(1);
  u_int64_t e2 = rdpmc_readCounter(2);
  u_int64_t e3 = rdpmc_readCounter(3);

  stats("test3", MAX_INTEGERS, e0-s0, e1-s1, e2-s2, e3-s3);
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

  int *ptr = (int*)malloc(sizeof(int)*MAX_INTEGERS);
  if (ptr==0) {
      fprintf(stderr, "Error: memory allocation failed\n");
      return 1;
  }

  printf("running pinned on CPU %d\n", cpu);

  test0(cpu);
  test1(cpu);
  test2(cpu, ptr);
  test3(cpu, ptr);

  return 0;
}
