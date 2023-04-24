#include <intel_pmu_stats.h>

std::ostream& Intel::Stats::print(std::ostream& stream) const {
  stream << "Intel XEON CPU HW Core "
         << d_pmu.coreId()
         << " PMU Summary on "
         << d_iterations
         << " iterations:"
         << std::endl;

  char buf[256];
  snprintf(buf, sizeof(buf), "%-3s [%-48s]: min: %012lu, max: %012lu, avg: %lf\n",
    "R0", "rdtsc cycles", d_rdtscMin, d_rdtscMax, (double)d_rdtscTotal/(double)d_iterations);
  stream << buf;

  for (u_int16_t i = 0; i<d_pmu.fixedCountersDefined(); ++i) {
    snprintf(buf, sizeof(buf), "%-3s [%-48s]: min: %012lu, max: %012lu, avg: %lf\n",
      d_pmu.fixedMnemonic()[i].c_str(),
      d_pmu.fixedDescription()[i].c_str(),
      d_fixedMin[i],
      d_fixedMax[i],
      (double)d_fixedTotal[i]/(double)d_iterations);
    stream << buf;
  }

  for (u_int16_t i = 0; i<d_pmu.programmableCountersDefined(); ++i) {
    snprintf(buf, sizeof(buf), "%-3s [%-48s]: min: %012lu, max: %012lu, avg: %lf\n",
      d_pmu.programmableMnemonic()[i].c_str(),
      d_pmu.programmableDescription()[i].c_str(),
      d_progMin[i],
      d_progMax[i],
      (double)d_progTotal[i]/(double)d_iterations);
    stream << buf;
  }

  return stream;
}

void Intel::Stats::recordFirstDatum() {
  const u_int64_t current = d_pmu.timeStampCounter();
  const u_int64_t delta   = current - d_rdtscLast;
  d_rdtscMin = d_rdtscMax = d_rdtscTotal = delta;
  d_rdtscLast = current;

  for (u_int16_t i=0; i<d_pmu.fixedCountersDefined(); ++i) {                                                              
    const u_int64_t current = d_pmu.fixedCounterValue(i);
    const u_int64_t delta   = current - d_fixedLast[i];
    d_fixedMin[i] = d_fixedMax[i] = delta;
    d_fixedLast[i] = current;
    d_fixedTotal[i] = delta;
  }                                                                                                                     

  for (u_int16_t i=0; i<d_pmu.programmableCountersDefined(); ++i) {                                                       
    const u_int64_t current = d_pmu.programmableCounterValue(i);
    const u_int64_t delta   = current - d_progLast[i];
    d_progMin[i] = d_progMax[i] = delta;
    d_progLast[i] = current;
    d_progTotal[i] = delta;
  }
}

void Intel::Stats::record() {
  if (0==d_iterations++) {
    recordFirstDatum(); 
    return;
  }

  const u_int64_t current = d_pmu.timeStampCounter();
  const u_int64_t delta   = current - d_rdtscLast;
  if (delta<d_rdtscMin) {
    d_rdtscMin = delta;
  } else if (delta>d_rdtscMax) {
    d_rdtscMax = delta;
  }
  d_rdtscLast = current;
  d_rdtscTotal += delta;

  for (u_int16_t i=0; i<d_pmu.fixedCountersDefined(); ++i) {                                                              
    const u_int64_t current = d_pmu.fixedCounterValue(i);
    const u_int64_t delta   = current - d_fixedLast[i];
    if (delta<d_fixedMin[i]) {
      d_fixedMin[i] = delta;
    } else if (delta>d_fixedMax[i]) {
      d_fixedMax[i] = delta;
    }
    d_fixedLast[i] = current;
    d_fixedTotal[i] += delta;
  }                                                                                                                     

  for (u_int16_t i=0; i<d_pmu.programmableCountersDefined(); ++i) {                                                       
    const u_int64_t current = d_pmu.programmableCounterValue(i);
    const u_int64_t delta   = current - d_progLast[i];
    if (delta<d_progMin[i]) {
      d_progMin[i] = delta;
    } else if (delta>d_progMax[i]) {
      d_progMax[i] = delta;
    }
    d_progLast[i] = current;
    d_progTotal[i] += delta;
  }
}
