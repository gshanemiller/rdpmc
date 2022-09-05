#include <intel_skylake_pmu.h>

void Intel::SkyLake::PMU::printSnapshot(const char *label) {
  u_int64_t ts = timeStampCounter();

  u_int64_t fixed[k_FIXED_COUNTERS];
  for (unsigned i=0; i<fixedCountersSupported(); ++i) {
    fixed[i] = fixedCounterValue(i);
  }

  u_int64_t prog[k_MAX_PROG_COUNTERS_HT_OFF];
  for (unsigned i=0; i<d_cnt; ++i) {
    prog[i] = programmableCounterValue(i);
  }

  bool fixedOverflow[k_FIXED_COUNTERS];
  for (unsigned i=0; i<fixedCountersSupported(); ++i) {
    fixedOverflow[i] = fixedCounterOverflowed(i);
  }
  
  bool progOverflow[k_MAX_PROG_COUNTERS_HT_OFF];
  for (unsigned i=0; i<d_cnt; ++i) {
    progOverflow[i] = programmableCounterOverflowed(i);
  }

  printf("%s: Intel::SkyLake CPU HW core: %d\n", label, core());
  printf("%-3s [%-60s]: value: %012lu\n", "C0", "rdtsc cycles: use with F2", ts - d_lastRdtsc);

  for (unsigned i = 0; i<fixedCountersSupported(); ++i) {
    printf("%-3s [%-60s]: value: %012lu, overflowed: %s\n",
      d_fixedMnemonic[i].c_str(),
      d_fixedDescription[i].c_str(),
      fixed[i],
      fixedOverflow[i] ? "true" : "false");
  }

  for (unsigned i = 0; i<d_cnt; ++i) {
    printf("%-3s [%-60s]: value: %012lu, overflowed: %s\n",
      d_progMnemonic[i].c_str(),
      d_progDescription[i].c_str(),
      prog[i],
      progOverflow[i] ? "true" : "false");
  }
}

int Intel::SkyLake::PMU::pinToHWCore(int coreId) {
  assert(coreId>=0);

  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(coreId, &mask);

  if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
      return errno;
  }

  return 0;
}
