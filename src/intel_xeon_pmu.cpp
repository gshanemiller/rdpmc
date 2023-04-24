#include <intel_xeon_pmu.h>

std::ostream& Intel::XEON::PMU::print(std::ostream& stream) const {
  u_int64_t ts = timeStampCounter();

  u_int64_t fixed[k_FIXED_COUNTERS];
  for (u_int16_t i=0; i<fixedCountersDefined(); ++i) {
    fixed[i] = fixedCounterValue(i);
  }

  u_int64_t prog[k_MAX_PROG_COUNTERS_HT_OFF];
  for (u_int16_t i=0; i<programmableCountersDefined(); ++i) {
    prog[i] = programmableCounterValue(i);
  }

  bool fixedOverflow[k_FIXED_COUNTERS];
  for (u_int16_t i=0; i<fixedCountersDefined(); ++i) {
    fixedOverflow[i] = fixedCounterOverflowed(i);
  }
  
  bool progOverflow[k_MAX_PROG_COUNTERS_HT_OFF];
  for (u_int16_t i=0; i<d_cnt; ++i) {
    progOverflow[i] = programmableCounterOverflowed(i);
  }

  stream << "Intel XEON CPU HW Core " << coreId() << " PMU Snapshot:" << std::endl;

  char buf[256];
  snprintf(buf, sizeof(buf), "%-3s [%-48s]: value: %012lu\n", "R0", "rdtsc cycles", ts);
  stream << buf;

  for (u_int16_t i = 0; i<fixedCountersDefined(); ++i) {
    snprintf(buf, sizeof(buf), "%-3s [%-48s]: value: %012lu, overflowed: %s\n",
      d_fixedMnemonic[i].c_str(),
      d_fixedDescription[i].c_str(),
      fixed[i],
      fixedOverflow[i] ? "true" : "false");
    stream << buf;
  }

  for (u_int16_t i = 0; i<programmableCountersDefined(); ++i) {
    snprintf(buf, sizeof(buf), "%-3s [%-48s]: value: %012lu, overflowed: %s\n",
      d_progMnemonic[i].c_str(),
      d_progDescription[i].c_str(),
      prog[i],
      progOverflow[i] ? "true" : "false");
    stream << buf;
  }

  return stream;
}

int Intel::XEON::PMU::pinToHWCore(int core) {
  assert(coreId>=0);

  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(core, &mask);

  if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
    int rc = errno;
    fprintf(stderr, "Error: cannot pin caller's thread to CPU HW core %d: %s\n", core, strerror(rc));
    return errno;
  }

  return 0;
}
