#include <intel_skylake_pmu.h>

using namespace Intel::SkyLake;

const int MAX_INTEGERS = 100000000;

void test0() {
  PMU pmu(false, PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);
  pmu.reset();
  pmu.start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  volatile int i=0;
  do {
    ++i;
  } while (__builtin_expect((i<MAX_INTEGERS),1));

  pmu.print("test loop no memory accesses");
}

void test1() {
  PMU pmu(false, PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);
  pmu.reset();
  pmu.start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  for (volatile int i=0; i<MAX_INTEGERS; i++);

  pmu.print("test loop no memory accesses");
}

void test2(int *ptr) {
  PMU pmu(false, PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);
  pmu.reset();
  pmu.start();

  // Memory heavily accessed, however, not randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    *(ptr+i) = 0xdeadbeef;
  }

  pmu.print("test loop accessing memory non-randomly");
}

void test3(int *ptr) {
  PMU pmu(false, PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);
  pmu.reset();
  pmu.start();

  // Memory heavily accessed randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    long idx = random() % MAX_INTEGERS;
    *(ptr+idx) = 0xdeadbeef;
  }

  pmu.print("test loop accessing memory randomly");
}

void test4() {
  PMU pmu(false, PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);
  pmu.reset();
  pmu.start();

  // No memory accessed
  unsigned s=0;
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    s+=1;
  }

  pmu.print("test loop no memory accesses");
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
  test4();

  free(ptr);
  ptr=0;

  return 0;
}
