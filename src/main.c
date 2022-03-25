#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>                                                                                                      
#include <rdpmc.h>

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

  rdpmc_initialize(cpu);
  rdpmc_snapshot();

  u_int64_t s0 = rdpmc_readCounter(0);
  u_int64_t s1 = rdpmc_readCounter(1);
  u_int64_t s2 = rdpmc_readCounter(2);
  u_int64_t s3 = rdpmc_readCounter(3);

  unsigned s=0;
  for (unsigned i=0; i<2000; i++) {
    s+=1;
  }

  u_int64_t e0 = rdpmc_readCounter(0);
  u_int64_t e1 = rdpmc_readCounter(1);
  u_int64_t e2 = rdpmc_readCounter(2);
  u_int64_t e3 = rdpmc_readCounter(3);

  printf("counter 0: %lu\n", e0-s0);
  printf("counter 1: %lu\n", e1-s1);
  printf("counter 2: %lu\n", e2-s2);
  printf("counter 3: %lu\n", e3-s3);

  rdpmc_close();

  return 0;
}
