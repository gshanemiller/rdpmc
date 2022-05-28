#include <intel_skylake_pmu.h>

void Intel::SkyLake::PMU::print(const char *label) {
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

  for (unsigned i = 0; i<fixedCountersSupported(); ++i) {
    printf("%-3s [%-40s]: value: %012lu, overflowed: %s\n",
      d_fixedMnemonic[i].c_str(),
      d_fixedDescription[i].c_str(),
      fixed[i],
      fixedOverflow[i] ? "true" : "false");
  }

  for (unsigned i = 0; i<d_cnt; ++i) {
    printf("%-3s [%-40s]: value: %012lu, overflowed: %s\n",
      d_progMnemonic[i].c_str(),
      d_progDescription[i].c_str(),
      prog[i],
      progOverflow[i] ? "true" : "false");
  }
}
