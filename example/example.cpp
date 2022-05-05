#include <intel_skylake_pmu.h>

using namespace Intel::SkyLake;

const int MAX_INTEGERS = 100000000;

void stats(const char *label, int iters, u_int64_t f0, u_int64_t f1, u_int64_t f2,
  u_int64_t p0, u_int64_t p1, u_int64_t p2, u_int64_t p3) {
  printf("-------------------------------------------------------------------\n");
  printf("%s: RAW VALUES\n", label);
  printf("-------------------------------------------------------------------\n");
  printf("%s:                : iterations run                : %d\n", label, iters);
  printf("%s: fixed counter 0: retired instructions          : %lu\n", label, f0);
  printf("%s: fixed counter 1: no-halt CPU cycles            : %lu\n", label, f1);
  printf("%s: fixed counter 2: ref no-halt CPU cycles        : %lu\n", label, f2);
  printf("%s: prog  counter 0: LLC references                : %lu\n", label, p0);
  printf("%s: prog  counter 1: LLC misses                    : %lu\n", label, p1);
  printf("%s: prog  counter 2: brch instrct retired          : %lu\n", label, p2);
  printf("%s: prog  counter 3: brch instrct not-taken retired: %lu\n", label, p3);
  printf("-------------------------------------------------------------------\n");
  printf("%s: DERIVED VALUES\n", label);
  printf("-------------------------------------------------------------------\n");
  printf("%s: retrd-ins/ref-cycle                            : %5.4lf\n", label, (double)(f0)/(double)(f2));
  printf("%s: retired instrct                per iteration   : %5.4lf\n", label, (double)(f0)/(double)(iters));
  printf("%s: LLC references                 per iteration   : %5.4lf\n", label, (double)(p0)/(double)(iters));
  printf("%s: LLC misses                     per iteration   : %5.4lf\n", label, (double)(p1)/(double)(iters));
  printf("%s: brch instrct retired           per iteration   : %5.4lf\n", label, (double)(p2)/(double)(iters));
  printf("%s: brch instrct not-taken retired per iteration   : %5.4lf\n", label, (double)(p3)/(double)(iters));
  printf("-------------------------------------------------------------------\n");
  printf("\n");
}

void test0() {
  PMU pmu(true, 0x414f2e, 0x41412e, 0x4104c4, 0x4110c4);
  pmu.reset();
  pmu.start();

  // No memory accessed:
  volatile int i=0;
  do {
    ++i;
  } while (__builtin_expect((i<MAX_INTEGERS),1));

  u_int64_t f0 = pmu.fixedCounterValue(0);
  u_int64_t f1 = pmu.fixedCounterValue(1);
  u_int64_t f2 = pmu.fixedCounterValue(2);

  u_int64_t p0 = pmu.programmableCounterValue(0);
  u_int64_t p1 = pmu.programmableCounterValue(1);
  u_int64_t p2 = pmu.programmableCounterValue(2);
  u_int64_t p3 = pmu.programmableCounterValue(3);

  stats("test0", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3);
}

void test1() {
  PMU pmu(true, 0x414f2e, 0x41412e, 0x4104c4, 0x4110c4);
  pmu.reset();
  pmu.start();

  // No memory accessed
  for (volatile int i=0; i<MAX_INTEGERS; i++);

  u_int64_t f0 = pmu.fixedCounterValue(0);
  u_int64_t f1 = pmu.fixedCounterValue(1);
  u_int64_t f2 = pmu.fixedCounterValue(2);

  u_int64_t p0 = pmu.programmableCounterValue(0);
  u_int64_t p1 = pmu.programmableCounterValue(1);
  u_int64_t p2 = pmu.programmableCounterValue(2);
  u_int64_t p3 = pmu.programmableCounterValue(3);

  stats("test1", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3);
}

void test2(int *ptr) {
  PMU pmu(true, 0x414f2e, 0x41412e, 0x4104c4, 0x4110c4);
  pmu.reset();
  pmu.start();

  // Memory heavily accessed, however, not randomly
  for (int i=0; i<MAX_INTEGERS; ++i) {
    *(ptr+i) = 0xdeadbeef;
  }

  u_int64_t f0 = pmu.fixedCounterValue(0);
  u_int64_t f1 = pmu.fixedCounterValue(1);
  u_int64_t f2 = pmu.fixedCounterValue(2);

  u_int64_t p0 = pmu.programmableCounterValue(0);
  u_int64_t p1 = pmu.programmableCounterValue(1);
  u_int64_t p2 = pmu.programmableCounterValue(2);
  u_int64_t p3 = pmu.programmableCounterValue(3);

  stats("test2", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3);
}

void test3(int *ptr) {
  PMU pmu(true, 0x414f2e, 0x41412e, 0x4104c4, 0x4110c4);
  pmu.reset();
  pmu.start();

  // Memory heavily accessed randomly
  for (int i=0; i<MAX_INTEGERS; ++i) {
    long idx = random() % MAX_INTEGERS;
    *(ptr+idx) = 0xdeadbeef;
  }

  u_int64_t f0 = pmu.fixedCounterValue(0);
  u_int64_t f1 = pmu.fixedCounterValue(1);
  u_int64_t f2 = pmu.fixedCounterValue(2);

  u_int64_t p0 = pmu.programmableCounterValue(0);
  u_int64_t p1 = pmu.programmableCounterValue(1);
  u_int64_t p2 = pmu.programmableCounterValue(2);
  u_int64_t p3 = pmu.programmableCounterValue(3);

  stats("test3", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3);
}

int main() {
  int *ptr = (int*)malloc(sizeof(int)*MAX_INTEGERS);
  if (ptr==0) {
      fprintf(stderr, "Error: memory allocation failed\n");
      return 1;
  }

  test0();
  test1();
  test2(ptr);
  test3(ptr);

  free(ptr);
  ptr=0;

  return 0;
}
