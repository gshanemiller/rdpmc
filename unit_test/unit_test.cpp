#include <gtest/gtest.h>

#include <intel_skylake_pmu.cpp>

using namespace Intel::SkyLake;

TEST(pmu, skylake) {  
  PMU pmu(false, 0x41412e);
  
  EXPECT_EQ(pmu.fixedCountersSupported(), 3);
  EXPECT_EQ(pmu.programmableCounterSupported(), 4);
  EXPECT_EQ(pmu.programmableCounterDefined(), 1);

  int core = -1;
  {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core, &mask);
    EXPECT_EQ(core, pmu.core());
  }
  // Inconclusive test that HW core never changes
  for (unsigned i=0; i<1000; ++i) {
    EXPECT_EQ(core, pmu.core());
  }

  // Counters should be 0 and stay that way 
  EXPECT_EQ(pmu.reset(), 0);
  for (unsigned i=0; i<10000; ++i) {
    for (int fixed = 0; fixed < pmu.fixedCountersSupported(); ++fixed) {
      EXPECT_EQ(pmu.fixedCounterValue(fixed), 0);
    }
    for (int pgm = 0; pgm < pmu.programmableCounterDefined(); ++pgm) {
      EXPECT_EQ(pmu.programmableCounterValue(pgm), 0);
    }
  }
    
  // Start the counters
  EXPECT_EQ(pmu.start(), 0);

  // Reset
  EXPECT_EQ(pmu.reset(), 0);
}

int main(int argc, char **argv) {                                                                                       
  testing::InitGoogleTest(&argc, argv);                                                                                 
  return RUN_ALL_TESTS();                                                                                               
}
