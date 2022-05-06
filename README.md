# rdpmc
Task: minimum, complete example demonstrating programming Intel's PMU to count CPU level events

# Acknowledgement
Motivated by [nanoBench](https://github.com/andreas-abel/nanoBench.git)

# Purpose
Use this library to micro-benchmark small sections of code by programming Intel's PMU (performance monitor unit)
to count selected events e.g. LLC cache misses, instructions retired.

# Advantages
* Minimum, complete
* Does not require `yum install msr-tools`
* Header only: include and you're done
* Works in user-space and/or kernel code
* Very low latency
* Programmable event types. Includes support for fixed counters
* Well documented
* Handles thread pinning if requested
* Simpler than [PAPI](https://icl.cs.utk.edu/papi/), [Nanobench](https://github.com/martinus/nanobench), and [PCM](https://github.com/opcm/pcm)
by one or two orders of 10. Now, to be fair, PCM does a heck of a lot more. But for benchmarking ADTs in which one is
principally intertested in cache misses and executions retired, this API is much more natural.

# Documentation
[See here for a fairly complete background on PMU programming](doc/pmu.md). This is among the very few documents that
pulls eveything together into one place. It should go a long way to removing the black-magic of PMU profiling.

# Test HW
* Tested on Equinix c3.small.x86 instance ($0.50/hr)
* Intel Xeon E-2278G @ 3.40Ghz
* Ubuntu 20.04 LTS
* Last tested May 2022

# Building
1. `git clone https://github.com/rodgarrison/rdpmc.git`
2. `cd rdpmc`
3. `mkdir build`
4. `cd build`
5. `cmake ..; make`
6. `sudo echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`; see limitations
7. `sudo ./perf.tsk`; see limitations for root motivation

# Limitations
1. Intel's PMU at least on the test HW can only track up to four to eight values per core at once depending on how you
count. If you need more measurements you'll need to run the code once for each set of metrics
2. You must run the code as root only because configuring the PMU is accomplished by opening and writing to device
files `/dev/cpu/<cpu>/msr` which is root protected by default
3. Prior to running the code you must run `echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`. This allows `rdpmc`
to run in user mode. Otherwise the code will segfault. Default value is `1` (kernel only). You only need to do this
once. Again, this is to work around Linux defaults
4. This code makes no attempt to store counter values in an array for later retrieval, however, that's a glorified
exercise in arrays
5. There will be some noise: if counters 0 is setup first then counters 1,2,3 counter 0 will see some the work for those
later counters as they are setup but before the test code runs. But this is a fixed amount. Therefore, benchmarking 
often runs the code under test several times to average the overhead out.
6. It's recommend to turn off hyper-threading. See doc for details.

# Example

```
#include <intel_skylake_pmu.h>

//
// Each programmable counter umask, event-select is anded with 0x410000 where
// 0x410000 enables bit 16 (USR space code) bit 22 (EN enable counter). See 
// 'doc/pmu.md' for details
//
const unsigned PMC0_CFG = 0x414f2e; // https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.REFERENCE
const unsigned PMC1_CFG = 0x41412e; // https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.MISS
const unsigned PMC2_CFG = 0x4104c4; // https://perfmon-events.intel.com/ -> SkyLake -> BR_INST_RETIRED.ALL_BRANCHES_PS
const unsigned PMC3_CFG = 0x4110c4; // https://perfmon-events.intel.com/ -> SkyLake -> BR_INST_RETIRED.COND_NTAKEN

void test0() {
  PMU pmu(true, PMC0_CFG, PMC1_CFG, PMC2_CFG, PMC3_CFG);
  pmu.reset();
  pmu.start();

  // No memory accessed. volatile helpe sure compiler does
  // not optimize out the loop into a no-op
  volatile int i=0;
  do {
    ++i;
  } while (__builtin_expect((i<MAX_INTEGERS),1));

  u_int64_t f0 = pmu.fixedCounterValue(0);
  u_int64_t f1 = pmu.fixedCounterValue(1);
  u_int64_t f2 = pmu.fixedCounterValue(2);

  u_int64_t p0 = pmu.programmableCounterValue(0);
  u_int64_t p1 = pmu.programmableCounterValue(1);
  u_int64_t p2 = pmu.programmableCounterValue(2);
  u_int64_t p3 = pmu.programmableCounterValue(3);

  u_int64_t overFlowStatus;
  pmu.overflowStatus(&overFlowStatus);

  // Pretty print status
  stats("test0", MAX_INTEGERS, f0, f1, f2, p0, p1, p2, p3, overFlowStatus);
}
```
