#include <gtest/gtest.h>

#include <intel_skylake_pmu.cpp>

using namespace Intel::SkyLake;

const u_int64_t PMC0_OVERFLOW_MASK = (1ull<<0);        // 'doc/intel_msr.pdf p287'
const u_int64_t PMC1_OVERFLOW_MASK = (1ull<<1);        // 'doc/intel_msr.pdf p287'
const u_int64_t PMC2_OVERFLOW_MASK = (1ull<<2);        // 'doc/intel_msr.pdf p287'
const u_int64_t PMC3_OVERFLOW_MASK = (1ull<<3);        // 'doc/intel_msr.pdf p287'
const u_int64_t FIXEDCTR0_OVERFLOW_MASK = (1ull<<32);  // 'doc/intel_msr.pdf p287'
const u_int64_t FIXEDCTR1_OVERFLOW_MASK = (1ull<<33);  // 'doc/intel_msr.pdf p288'
const u_int64_t FIXEDCTR2_OVERFLOW_MASK = (1ull<<34);  // 'doc/intel_msr.pdf p288'

//
// Each programmable counter umask, event-select is anded with 0x410000 where
// 0x410000 enables bit 16 (USR space code) bit 22 (EN enable counter). See
// 'doc/pmu.md' for details
//
const unsigned PMC0_CFG = 0x414f2e;                    // https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.REFERENCE
const unsigned PMC1_CFG = 0x41412e;                    // https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.MISS
const unsigned PMC2_CFG = 0x4104c4;                    // https://perfmon-events.intel.com/ -> SkyLake -> BR_INST_RETIRED.ALL_BRANCHES_PS
const unsigned PMC3_CFG = 0x4110c4;                    // https://perfmon-events.intel.com/ -> SkyLake -> BR_INST_RETIRED.COND_NTAKEN

TEST(pmu, skylake) {  
  // Create PMU object running all fixed counters and 
  // four programmable counters with specified configs
  // Request with arg0 'true' that thread is pinned to
  // core which is required for API to work reliably
  PMU pmu(true, PMC0_CFG, PMC1_CFG, PMC2_CFG, PMC3_CFG);
  
  EXPECT_EQ(pmu.fixedCountersSupported(), 3);
  EXPECT_EQ(pmu.programmableCounterSupported(), 4);
  EXPECT_EQ(pmu.programmableCounterDefined(), 4);
  // 0x222 = SkyLake::PMU::DEFAULT_FIXED_CONFIG
  EXPECT_EQ(pmu.fixedCounterConfiguration(), 0x222);

  // Core holds current HW core for current thread
  int core = pmu.core();
  // Inconclusive test that HW core never changes
  for (unsigned i=0; i<1000; ++i) {
    EXPECT_EQ(core, pmu.core());
  }

  // Reset and configure all requested counters
  EXPECT_EQ(pmu.reset(), 0);

  // Counters should be 0 and stay that way since not started
  for (unsigned i=0; i<10000; ++i) {
    for (unsigned fixed = 0; fixed < pmu.fixedCountersSupported(); ++fixed) {
      EXPECT_EQ(pmu.fixedCounterValue(fixed), 0);
    }
    for (unsigned pgm = 0; pgm < pmu.programmableCounterDefined(); ++pgm) {
      EXPECT_EQ(pmu.programmableCounterValue(pgm), 0);
    }
  }

  // Overflow status should be 0 since not started
  u_int64_t status;
  EXPECT_EQ(pmu.overflowStatus(&status), 0);
  EXPECT_EQ(status, 0);
    
  // Start counters
  EXPECT_EQ(pmu.start(), 0);

  // Reset
  EXPECT_EQ(pmu.reset(), 0);

  // Counters should be 0 and stay that way since not started
  for (unsigned i=0; i<10000; ++i) {
    for (unsigned fixed = 0; fixed < pmu.fixedCountersSupported(); ++fixed) {
      EXPECT_EQ(pmu.fixedCounterValue(fixed), 0);
    }
    for (unsigned pgm = 0; pgm < pmu.programmableCounterDefined(); ++pgm) {
      EXPECT_EQ(pmu.programmableCounterValue(pgm), 0);
    }
  }

  // Overflow status should be 0 since not started
  EXPECT_EQ(pmu.overflowStatus(&status), 0);
  EXPECT_EQ(status, 0);
}

int main(int argc, char **argv) {                                                                                       
  testing::InitGoogleTest(&argc, argv);                                                                                 
  return RUN_ALL_TESTS();                                                                                               
}
