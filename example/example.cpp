#include <intel_xeon_pmu.h>
#include <intel_pmu_stats.h>

#include <algorithm>

#include <time.h>
#include <unistd.h>

using namespace Intel::XEON;

const int MAX_INTEGERS = 100000000;

PMU *pmu = 0;

void test0() {
  pmu->reset();
  pmu->start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  volatile int i=0;
  do {
    ++i;
  } while (__builtin_expect((i<MAX_INTEGERS),1));

  std::cout << "test loop no memory accesses" << std::endl;
  std::cout << *pmu << std::endl;
}

void test1() {
  pmu->reset();
  pmu->start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  for (volatile int i=0; i<MAX_INTEGERS; i++);

  std::cout << "test loop no memory accesses" << std::endl;
  std::cout << *pmu << std::endl;
}

void test2(int *ptr) {
  pmu->reset();
  pmu->start();

  // Memory heavily accessed, however, not randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    *(ptr+i) = 0xdeadbeef;
  }

  std::cout << "test loop accessing memory non-randomly" << std::endl;
  std::cout << *pmu << std::endl;
}

void test3(int *ptr) {
  pmu->reset();
  pmu->start();

  // Memory heavily accessed randomly
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    long idx = random() % MAX_INTEGERS;
    *(ptr+idx) = 0xdeadbeef;
  }

  std::cout << "test loop accessing memory randomly" << std::endl;
  std::cout << *pmu << std::endl;
}

void test4() {
  pmu->reset();
  pmu->start();

  // No memory accessed
  unsigned s=0;
  for (volatile int i=0; i<MAX_INTEGERS; ++i) {
    s+=1;
  }

  std::cout << "test loop accessing no memory" << std::endl;
  std::cout << *pmu << std::endl;
}

void testStats(int *ptr) {
  Intel::Stats stats(*pmu);

  pmu->reset();
  pmu->start();
  stats.reset();

  // Run test three times
  for (unsigned runs=0; runs<3; ++runs) {

    // Memory heavily accessed randomly
    for (volatile int i=0; i<MAX_INTEGERS; ++i) {
      long idx = random() % MAX_INTEGERS;
      *(ptr+idx) = 0xdeadbeef;
    }

    stats.record();
  }

  // Dump PMU data one set per run
  std::cout << stats << std::endl;
}

int main() {
  pmu = new PMU(PMU::k_DEFAULT_XEON_CONFIG_0);

  int *ptr = (int*)malloc(sizeof(int)*MAX_INTEGERS);
  if (ptr==0) {
      fprintf(stderr, "Error: memory allocation failed\n");
      return 1;
  }

  // Different tests snap shotting counters
  test0();
  test1();
  test2(ptr);
  test3(ptr);
  test4();

  // Test simple stats:
  testStats(ptr);

  free(ptr);
  ptr=0;
  delete pmu;
  pmu = 0;

  return 0;
}
