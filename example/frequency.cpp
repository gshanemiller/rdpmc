#include <intel_skylake_pmu.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

using namespace Intel::SkyLake;
PMU pmu(false, PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);

const u_int64_t secToNs   = 1000000000UL;
const u_int64_t target    = 1000000000UL;
const u_int64_t delta     = 75000UL;
const u_int64_t epsilonNs = 100UL;
const double    hzToGhz   = 1000000000.0;

int main() {
  struct timespec startTs, endTs;
  u_int64_t       startRdtsc, endRdtsc;
  u_int64_t       iterations = 1000000000;
  u_int64_t       elapsedNs;
  u_int64_t       elapsedRdtsc;

  while (1) {
	  clock_gettime(CLOCK_MONOTONIC, &startTs);
    startRdtsc = pmu.timeStampCounter();
    for (volatile u_int64_t i=0; i<iterations; ++i);
    endRdtsc = pmu.timeStampCounter();
	  clock_gettime(CLOCK_MONOTONIC, &endTs);

    assert(endTs.tv_sec>=startTs.tv_sec);
    if (endTs.tv_sec<startTs.tv_sec) {
      fprintf(stderr, "discarding run (rare)\n");
      continue;
    }

    elapsedNs = endTs.tv_sec*secToNs+endTs.tv_nsec - (startTs.tv_sec*secToNs+startTs.tv_nsec);
    elapsedRdtsc = endRdtsc - startRdtsc;
    double freqGhz = (double)elapsedRdtsc / hzToGhz;
    double nsPerRdtscCycle = 1.0 / (double)elapsedRdtsc * (double)secToNs;

    fprintf(stderr, "iterations %lu, elapsedTimeNs %lu, elapsedRdtsc %lu, freqGhz %lf -> 1-rdtsc-cycle = %lf ns\n",
      iterations, elapsedNs, elapsedRdtsc, freqGhz, nsPerRdtscCycle);

    if (elapsedNs>target) {
      if ((elapsedNs-target)<=epsilonNs) {
        break;
      }
      iterations -= delta;
    } else {
      if ((target-elapsedNs)<=epsilonNs) {
        break;
      }
      iterations += delta;
    }
  }

  return 0;
}
