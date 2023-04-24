#pragma once

// PURPOSE: Cheaply summarize PMU data by counter
//
// CLASSES:
//  Intel::Stats: Provide min/max/avg by counter. Average is computed equivalent to (end-start)/iterations by counter.
//                Note this class does not check for overflow when computing values

#include <intel_xeon_pmu.h>

namespace Intel {

class Stats {
  // DATA
  u_int64_t d_fixedMin[XEON::PMU::k_FIXED_COUNTERS];            // minimum relative value by fixed counter
  u_int64_t d_fixedMax[XEON::PMU::k_FIXED_COUNTERS];            // maximum relative value by fixed counter
  u_int64_t d_progMin[XEON::PMU::k_MAX_PROG_COUNTERS_HT_OFF];   // minimum relative value by programmable counter
  u_int64_t d_progMax[XEON::PMU::k_MAX_PROG_COUNTERS_HT_OFF];   // maximum relative value by programmable counter
  u_int64_t d_fixedLast[XEON::PMU::k_FIXED_COUNTERS];           // last absolute fixed counter value
  u_int64_t d_progLast[XEON::PMU::k_MAX_PROG_COUNTERS_HT_OFF];  // last absolute programmable counter value
  u_int64_t d_fixedTotal[XEON::PMU::k_FIXED_COUNTERS];          // running sum of relative values by fixed counter
  u_int64_t d_progTotal[XEON::PMU::k_MAX_PROG_COUNTERS_HT_OFF]; // running sum of relative values by prog counter
  u_int64_t d_rdtscMin;                                         // minimum relative value of rdtsc counter
  u_int64_t d_rdtscMax;                                         // maximum relative value of rdtsc counter
  u_int64_t d_rdtscLast;                                        // last absolute value of rdtsc timer
  u_int64_t d_rdtscTotal;                                       // running sum of relative rdtsc values
  u_int64_t d_iterations;                                       // number of times 'record' called
  const Intel::XEON::PMU& d_pmu;                                // the PMU object providing counter values

  // CREATORS
public:
  Stats(const Intel::XEON::PMU& pmu);
    // Create a Stats object collecting statistics from specified 'pmu'. Callers must call reset()

  Stats(const Stats& other) = delete;
    // Copy constructor not defined

  ~Stats() = default;
    // Destroy this object

  // MANIPULATORS
  void record();
    // Update internal state by reading the current value of all defined counters from PMU object provided at
    // construction time. Behavior is defined if 'pmu' was successfully started, and 'reset()' run before recording
    // starts.

  void reset();
    // Reset collected state reflecting 0 recorded samples.

  Stats& operator=(const Stats& rhs) = delete;
    // Assignment operator not provided
  
  // ASPECTS
  std::ostream& print(std::ostream& stream) const;
    // Pretty print to specified 'stream' min/max/avg by counter for all data collected through last call to 'record'.

  // PRIVATE MANIPULATORS
  void recordFirstDatum();
    // Special case for 'record' when d_iterations==0
};

// FREE OPERATORS
std::ostream& operator<<(std::ostream& stream, const Stats& object);
    // Pretty print to specified 'stream' min/max/avg by counter for all data collected through last call to 'record'
    // in specified 'object'

// INLINE DEFINITIONS
// CREATORS
inline
Stats::Stats(const Intel::XEON::PMU& pmu)
: d_pmu(pmu)
{
  reset();
}

// INLINE DEFINITIONS
// MANIPULATORS
inline
void Stats::reset() {
  d_iterations = 0;
  d_rdtscTotal = 0;

  memset(d_fixedTotal, 0, sizeof(d_fixedTotal));
  memset(d_progTotal,  0, sizeof(d_progTotal));

  d_rdtscLast = d_pmu.timeStampCounter();

  for (u_int16_t i=0; i<d_pmu.fixedCountersDefined(); ++i) {
    d_fixedLast[i] = d_pmu.fixedCounterValue(i);
  }

  for (u_int16_t i=0; i<d_pmu.programmableCountersDefined(); ++i) {
    d_progLast[i] = d_pmu.programmableCounterValue(i);
  }
}

// INLINE DEFINITIONS
// FREE OPERATORS
inline
std::ostream& operator<<(std::ostream& stream, const Stats& object) {
  return object.print(stream);
}

} // namespace Intel
