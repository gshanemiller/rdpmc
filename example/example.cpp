#include <intel_skylake_pmu.h>

using namespace Intel::SkyLake;

const int MAX_INTEGERS = 100000000;

const u_int64_t PMC0_OVERFLOW_MASK = (1ull<<0);        // 'doc/intel_msr.pdf p287'
const u_int64_t PMC1_OVERFLOW_MASK = (1ull<<1);        // 'doc/intel_msr.pdf p287'
const u_int64_t PMC2_OVERFLOW_MASK = (1ull<<2);        // 'doc/intel_msr.pdf p287'
const u_int64_t PMC3_OVERFLOW_MASK = (1ull<<3);        // 'doc/intel_msr.pdf p287'
const u_int64_t FIXEDCTR0_OVERFLOW_MASK = (1ull<<32);  // 'doc/intel_msr.pdf p287'
const u_int64_t FIXEDCTR1_OVERFLOW_MASK = (1ull<<33);  // 'doc/intel_msr.pdf p288'
const u_int64_t FIXEDCTR2_OVERFLOW_MASK = (1ull<<34);  // 'doc/intel_msr.pdf p288'

void stats(const char *label, int iters, u_int64_t f0, u_int64_t f1, u_int64_t f2,
  u_int64_t p0, u_int64_t p1, u_int64_t p2, u_int64_t p3, u_int64_t overFlowStatus) {
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
  printf("%s: OVERFLOW STATUS\n", label);
  printf("-------------------------------------------------------------------\n");
  printf("%s: fixed counter 0: overflow status               : %lu\n", label, overFlowStatus & FIXEDCTR0_OVERFLOW_MASK);
  printf("%s: fixed counter 1: overflow status               : %lu\n", label, overFlowStatus & FIXEDCTR1_OVERFLOW_MASK); 
  printf("%s: fixed counter 2: overflow status               : %lu\n", label, overFlowStatus & FIXEDCTR2_OVERFLOW_MASK); 
  printf("%s: prog  counter 0: overflow status               : %lu\n", label, overFlowStatus & PMC0_OVERFLOW_MASK);
  printf("%s: prog  counter 1: overflow status               : %lu\n", label, overFlowStatus & PMC1_OVERFLOW_MASK);
  printf("%s: prog  counter 2: overflow status               : %lu\n", label, overFlowStatus & PMC2_OVERFLOW_MASK);
  printf("%s: prog  counter 3: overflow status               : %lu\n", label, overFlowStatus & PMC3_OVERFLOW_MASK);
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

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
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

  u_int64_t overFlowStatus;
  pmu.overflowStatus(&overFlowStatus);

  stats("test0", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3, overFlowStatus);
}

void test1() {
  PMU pmu(true, 0x414f2e, 0x41412e, 0x4104c4, 0x4110c4);
  pmu.reset();
  pmu.start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  for (volatile int i=0; i<MAX_INTEGERS; i++);

  u_int64_t f0 = pmu.fixedCounterValue(0);
  u_int64_t f1 = pmu.fixedCounterValue(1);
  u_int64_t f2 = pmu.fixedCounterValue(2);

  u_int64_t p0 = pmu.programmableCounterValue(0);
  u_int64_t p1 = pmu.programmableCounterValue(1);
  u_int64_t p2 = pmu.programmableCounterValue(2);
  u_int64_t p3 = pmu.programmableCounterValue(3);

  u_int64_t overFlowStatus;
  pmu.overflowStatus(&overFlowStatus);

  stats("test1", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3, overFlowStatus);
}

void test2(int *ptr) {
  PMU pmu(true, 0x414f2e, 0x41412e, 0x4104c4, 0x4110c4);
  pmu.reset();
  pmu.start();

  // Memory heavily accessed, however, not randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    *(ptr+i) = 0xdeadbeef;
  }

  u_int64_t f0 = pmu.fixedCounterValue(0);
  u_int64_t f1 = pmu.fixedCounterValue(1);
  u_int64_t f2 = pmu.fixedCounterValue(2);

  u_int64_t p0 = pmu.programmableCounterValue(0);
  u_int64_t p1 = pmu.programmableCounterValue(1);
  u_int64_t p2 = pmu.programmableCounterValue(2);
  u_int64_t p3 = pmu.programmableCounterValue(3);

  u_int64_t overFlowStatus;
  pmu.overflowStatus(&overFlowStatus);

  stats("test2", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3, overFlowStatus);
}

void test3(int *ptr) {
  PMU pmu(true, 0x414f2e, 0x41412e, 0x4104c4, 0x4110c4);
  pmu.reset();
  pmu.start();

  // Memory heavily accessed randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
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

  u_int64_t overFlowStatus;
  pmu.overflowStatus(&overFlowStatus);

  stats("test3", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3, overFlowStatus);
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
