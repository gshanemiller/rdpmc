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
* Works in user-space and/or kernel code
* Very low latency
* Programmable event types
* Includes support for fixed counters
* Overflow detector
* Well documented
* Handles thread pinning if requested
* Simpler than [PAPI](https://icl.cs.utk.edu/papi/), [Nanobench](https://github.com/martinus/nanobench), and [PCM](https://github.com/opcm/pcm)
by one or two orders of 10. Now, to be fair, PCM does a heck of a lot more. But for benchmarking ADTs in which one is
principally intertested in cache misses and executions retired, this API is much more natural.

# Documentation
[See here for a fairly complete background on PMU programming](doc/pmu.md). This is among the very few documents that
pulls everything together into one place. It should go a long way to removing the black-magic of PMU profiling.

# Test HW
* Tested on Equinix c3.small.x86 instance ($0.50/hr)
* Intel Xeon E-2278G @ 3.40Ghz
* Ubuntu 20.04 LTS
* Last tested May 2022

# Building and Running
1. `git clone https://github.com/rodgarrison/rdpmc.git`
2. `cd rdpmc`
3. `mkdir build`
4. `cd build`
5. `cmake ..; make`
6. `sudo echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`; see limitations
7. `sudo taskset -c 0 ./example/example.tsk`; see limitations for root motivation and 'Example section for taskset

# Limitations
1. Intel's PMU at least on the test HW can only track up to eight programmable values per core at once. If CPU hyper
threading is OFF, four distinct concurrent programmable counters can be configured otherwise 8 **per HW core**. Three
fixed counters are always available, configured, and run. If you need more measurements you'll need to run the code
once for each set of metrics with a different config
2. You must run the code as root only because configuring the PMU is accomplished by opening and writing to device
files `/dev/cpu/<cpu>/msr` which is root protected by default. This methodology is praxis
3. Prior to running the code you must run `echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`. This allows `rdpmc`
to run in user mode. Otherwise the code will segfault. Default value is `1` (kernel only). You only need to do this
once. This is to work around for Linux defaults. A helper script is provided in the also `scripts` subdir for 
this purpose.
4. This code makes no attempt to store counter values in an array for later retrieval, however, that's a glorified
exercise in arrays
5. There will be some noise: if counters 0 is setup first then counters 1,2,3 counter 0 will see some the work for
those later counters as they are setup but before the test code runs. But this is a fixed amount. This is unavoidable.
PMU metrics are extremely low latency and responsive. Therefore, benchmarking often runs the code under test several
times to average the overhead out.
6. Requesting and running more counters than are allowed depending on whether HT is on/off is undefined behavior.
This library does not check or enforce a limit.
7. While not a limitation per se, PMU results are undefined if the instrumented code is not pinned to a HW core while
running. PMU counters are by construction per core counters only. PMU will not follow the code core-to-core as the O/S
bounces it around.

# Example

```
#include <intel_skylake_pmu.h>

void test1() {
  PMU pmu(false, PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);
  pmu.reset();
  pmu.start();

  // No memory accessed. volatile tells compiler
  // to not optimize out the loop into a no-op
  for (volatile int i=0; i<MAX_INTEGERS; i++);

  pmu.print("test loop no memory accesses");
}

$ taskset -c 0 ./test.tsk
test loop no memory accesses: Intel::SkyLake CPU HW core: 0
F0  [retired instructions                    ]: value: 000600000168, overflowed: false
F1  [no-halt cpu cycles                      ]: value: 000500610283, overflowed: false
F2  [reference no-halt cpu cycles            ]: value: 000342054144, overflowed: false
P0  [LLC references                          ]: value: 000000000015, overflowed: false
P1  [LLC misses                              ]: value: 000000000000, overflowed: false
P2  [retired branch instructions             ]: value: 000100000044, overflowed: false
P3  [retired branch instructions not taken   ]: value: 000000000004, overflowed: false
```

Note that while the API can pin code to a CPU core, the example code passes 'false'
as the first argument when constructing 'pmu' because it's assumed the task will be
invoked 'taskset' pinned to a core already. Feel free to change set 'true' so that
'taskset' is not required. The advantage of taskset is that caller chooses core.

# Scripts
The scripts directory provides three trivial bash scripts:

* **intel_ht**: sudo run with argument `on|off`. This enables or disables Intel HW Core hyper-threading. It's
recommended to turn it off for PMU work. See `doc/pmu.md`. If on you risk other HW threads running on your test
core leaking into your PMU measurements.
* **linux_nmi**: sudo run with argument `on|off`. This disables NMI interrupts recommended during PMU work. Per
Nanobench, NMI uses a counter. **Not tested or validated** 
* **linux_pmu**: sudo run with no arguments. This allows `rdpmc` assembler instruction to be called in userspace
code. It's mandatory to run this before doing PMU code.
