#include <intel_skylake_pmu.h>

using namespace Intel::SkyLake;

const int MAX_INTEGERS = 100000000;

void stats(const char *label, int iters, u_int64_t f0, u_int64_t f1, u_int64_t f2,
  u_int64_t p0, u_int64_t p1, u_int64_t p2, u_int64_t p3) {
  printf("%s:                : iterations run      : %d\n", label, iters);
  printf("%s: fixed counter 0: unhalted core cycles: %lu\n", label, f0);
  printf("%s: fixed counter 1: retired instructions: %lu\n", label, f1);
  printf("%s: fixed counter 2: LLC references      : %lu\n", label, f2);
  printf("%s: prog  counter 0: LLC references      : %lu\n", label, p0);
  printf("%s: prog  counter 1: LLC references      : %lu\n", label, p1);
  printf("%s: prog  counter 2: LLC references      : %lu\n", label, p2);
  printf("%s: prog  counter 3: LLC references      : %lu\n", label, p3);
  printf("%s: retrd-ins/cycle: %4.3lf\n", label, (double)(f0)/(double)(f1));
  printf("\n");
}

void test0() {
  PMU pmu(true, 0x41003c, 0x4100c0, 0x414f2e, 0x41412e);
  pmu.reset();
  pmu.start();

  // No memory accessed:
  int i=0;
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
  PMU pmu(true, 0x41003c, 0x4100c0, 0x414f2e, 0x41412e);
  pmu.reset();
  pmu.start();

  // No memory accessed
  for (int i=0; i<MAX_INTEGERS; i++);

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
  PMU pmu(true, 0x41003c, 0x4100c0, 0x414f2e, 0x41412e);
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
  PMU pmu(true, 0x41003c, 0x4100c0, 0x414f2e, 0x41412e);
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
  return 0;
}
