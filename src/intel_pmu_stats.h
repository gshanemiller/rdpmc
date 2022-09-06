#pragma once

// PURPOSE: Report collected statistics
//
// CLASSES:
//  Intel::Stats: Holds raw statistics from each test run reporting them to standard out.

#include <string>
#include <vector>
#include <iostream>
#include <inttypes.h>
#include <time.h>

namespace Intel {

// forward decl
namespace SkyLake {
  class PMU;
}

class Stats {
  // DATA
  std::vector<std::string>    d_description;  // per result set: description e.g. 'insert in cuckoo hashmap'
  std::vector<u_int64_t>      d_itertions;    // per result set: number of iterations
  std::vector<u_int64_t>      d_rdstc;        // per result set: elasped rdstc at test end
  std::vector<u_int64_t>      d_fixedCntr0;   // per result set: elapsed value of fixed counter 0 at test end
  std::vector<u_int64_t>      d_fixedCntr1;   // per result set: elapsed value of fixed counter 1 at test end
  std::vector<u_int64_t>      d_fixedCntr2;   // per result set: elapsed value of fixed counter 2 at test end
  std::vector<u_int64_t>      d_progmCntr0;   // per result set: elapsed value of programmable counter 0 at test end
  std::vector<u_int64_t>      d_progmCntr1;   // per result set: elapsed value of programmable counter 1 at test end
  std::vector<u_int64_t>      d_progmCntr2;   // per result set: elapsed value of programmable counter 2 at test end
  std::vector<u_int64_t>      d_progmCntr3;   // per result set: elapsed value of programmable counter 3 at test end
  std::vector<u_int64_t>      d_progmCntr4;   // per result set: elapsed value of programmable counter 4 at test end
  std::vector<u_int64_t>      d_progmCntr5;   // per result set: elapsed value of programmable counter 5 at test end
  std::vector<u_int64_t>      d_progmCntr6;   // per result set: elapsed value of programmable counter 6 at test end
  std::vector<u_int64_t>      d_progmCntr7;   // per result set: elapsed value of programmable counter 7 at test end
  std::vector<double>         d_elapsedNs;    // per result set: elapsed time in nanoseconds

  // CREATORS
public:
  Stats() = default;
    // Create a Stats object containing no data

  Stats(const Stats& other) = delete;
    // Copy constructor not defined

  ~Stats() = default;
    // Destroy this object

  // MANIPULATORS
  void record(const char *desc,
              u_int64_t iterations,
              timespec start,
              timespec end,
              const Intel::SkyLake::PMU& pmu);
    // Record the current value of each enabled fixed and programmable counter plus 'rdstc' defined in specified 'pmu'.
    // In addition associate with the result set a description of the data with specified 'desc', specified 'iterations'
    // describing how many operations were run e.g. inserts, loops, finds, adds etc., and the elapsed time specified as
    // 'end - start'. Behavior is defined provided 'iterations>0', and 'pmu' was successfully started.

  void reset();
    // Discard all collected results

  Stats& operator=(const Stats& rhs) = delete;
    // Assignment operator not provided

private:
  // PRIVATE MANIPULATORS
  void calcMinMaxAvgData(const std::vector<u_int64_t>& data, const std::vector<u_int64_t>& iterations,
    double& min, double& max, double& avg) const;
    // Calculate the minimum, maximum, and average statistics using specified 'data, iterations' writing results into
    // specified 'min, max, avg'. 'min' is defined as the smallest value in data divided by the number of iterations
    // in that same run. 'max' is defined similarly. 'avg' is defined as the total of all entries in 'data' divided
    // by the total of all entries in 'iterations'.

  void calcMinMaxAvgTime(const std::vector<double>& elapsedNs, const std::vector<u_int64_t>& iterations,
    double ns[3], double nsPerIter[3], double ops[3], double iters[3]) const;
    // Calculate the minimum, maximum, and average statistics using specified 'elapsedNs, iterations' writing results
    // into specified 'ns, nsPerIter, ops, iters'. Each double array holds min at index 0, max index 1, avg at index 2.
    // 'min' is defined as the smallest value in data divided by the number of iterations in that same run. 'max' is
    // defined similarly. 'avg' is defined as the total of all entries in 'data' divided by the total of all entries
    // in 'iterations'.

public:
  // ASPECTS
  void legend(const Intel::SkyLake::PMU& pmu) const;
    // Print to stdout a legend of all counters enabled in specified 'pmu'

  void dump(const Intel::SkyLake::PMU& pmu) const;
    // Print to stdout a human readable dump of all results collected with 'record' via specified 'pmu'

  void dumpScaled(const Intel::SkyLake::PMU& pmu) const;
    // Print to stdout a human readable dump of all results collected with 'record' via specified 'pmu' scaled by
    // dividing each counter value by the number of iterations given at 'record' time

  void summary(const char *label, const Intel::SkyLake::PMU& pmu) const;
    // Print to stdout a human readable summary of all results collected with 'record' via specified 'pmu'. For each
    // counter the min, max, and avgerage scaled value is provided. Specified 'label' gives summary context; the per
    // result description is not printed while summarizing
};

// INLINE DEFINITIONS
// MANIPULATORS
inline
void Stats::reset() {
  d_description.clear();
  d_itertions.clear();
  d_rdstc.clear();
  d_fixedCntr0.clear();
  d_fixedCntr1.clear();
  d_fixedCntr2.clear();
  d_progmCntr0.clear();
  d_progmCntr1.clear();
  d_progmCntr2.clear();
  d_progmCntr3.clear();
  d_progmCntr4.clear();
  d_progmCntr5.clear();
  d_progmCntr6.clear();
  d_progmCntr7.clear();
  d_elapsedNs.clear();
}

} // namespace Benchmark
