# rdpmc
Library: minimum, complete example demonstrating programming Intel's PMU to count CPU level events

# Acknowledgement
Motivated by [nanoBench](https://github.com/andreas-abel/nanoBench.git)

# Purpose
Use this library to micro-benchmark small sections of code by programming Intel's PMU (performance monitor unit)
to count selected events e.g. LLC cache misses, instructions retired using Intel PMU hardware counters.

# Advantages
* Low latency
* Minimal and complete. No externel dependencies required
* Works in user-space and/or kernel code
* Programmable event types
* Includes support for fixed counters
* Reports rdtsc values
* Counter overflow detection
* Well documented
* Code as-shipped works for PMU versions 3,4,5 e.g. Skylake and later
* Provides helper class to collect PMU stats and summarize
* Simpler than [PAPI](https://icl.cs.utk.edu/papi/), [Nanobench](https://github.com/martinus/nanobench), and [PCM](https://github.com/opcm/pcm)
by one or two orders of ten. Now, to be fair, PCM does a heck of a lot more. But for benchmarking typical programming
tasks e.g. hashmap insert, qsort, or matrix-multiply this API is far simpler.

# Documentation
[See here for a fairly complete background on PMU programming](doc/pmu.md). This is among the very few documents that
pulls everything together into one place. It should go a long way to removing the black-magic of PMU profiling.

# Test HW
* Tested on Equinix c3.small.x86 instance ($0.75/hr)
* Intel Xeon E-2278G @ 3.40Ghz
* Ubuntu 20.04 LTS, RHEL-8
* Last tested Apr 2023

# Building
1. `git clone https://github.com/rodgarrison/rdpmc.git`
2. `cd rdpmc`
3. `mkdir build`
4. `cd build`
5. `cmake ..; make`

# Running Example
1. cd `rdpmc/build`
2. `sudo echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`
3. If running as non-root (see #Configuration below): `sudo setcap cap_sys_rawio,cap_dac_override=epi ./example/example.tsk`
4. `taskset -c 1 ./example/example.tsk`

# Limitations
1. Intel's PMU at least on the test HW can only track up to eight programmable values per core at once if CPU hyper
threading is OFF and four if CPU hyper threading is ON **per core**. Three fixed counters **per core** are always
available, configured, and run. If you need more measurements (7 at once HT on or 11 at once if HT off) you'll need
to run the code once for each distinct set of metrics.
2. There will be some noise: if counters 0 is setup first then counters 1,2,3 counter 0 will see some the work for
later counters as they are started but before the test code runs. This is unavoidable. Setting up a counter requires
writing configurations to MSR registers over a file handle. To remove noise run your code multiple times, and take
averages. In the alternative setup and start the counters in the usual way, then when done, baseline the starting
values by taking a snapshot. Now run the test code, and take counter differences from the baseline. Combine with
averaging. Again, while the PMU counters might see a bit of the code to read PMU counter values it's considerably
less that setup. Counter reads require a few assembler instructions.
3. Requesting and running more counters than allowed depending on whether HT is on/off is undefined behavior. This
library does not check or enforce a limit.
4. Programming events not supported on the PMU hardware is not detected. That's also undefined behavior.
5. While not a limitation per se, PMU results are undefined if the test code is not pinned to a HW core while
running. PMU counters are by construction per core counters only. PMU does not follow your thread as it bounces
around core-to-core. To help avoid these problems, the PMU constructor unconditionally pins itself to the caller's
current core at construction time. If the task running the code was already pinned at PMU construction time or was
run taskset, this behavior will have no effect.
6. By default Linux does not allow non-root users access to PMU countets. See #Configuration to fix that.

#Configuration
Whether or not you run PMU profiling code as root or no you must must enable the assembler instruction `rdpmc` to
run in user code (ring 2). By default Linux only allows it to run in kernel mode (ring 0). `rdpmc` reads a counter
value. This only needs to be done once after each boot: `sudo echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`.
If you'll only run PMU code in kernel mode, this step is not required.

If you run your PMU profiling code as a non-root user, you must allow the code to write to `/dev/cpu/*/msr` system
files which configure the counters during setup. By default Linux does not allow this for non-root users. The
easist way is `sudo setcap cap_sys_rawio,cap_dac_override=epi <path-to-your-exe>`. This command allows the named
task to **open** the MSR system (`cap_sys_rawio`) files and read/write the MSR files (`cap_dac_override`) even
though the whole `/dev` tree is root protected. You may remove `cap_dac_override` by chmoding `/dev/cpu/*/msr` to
be read/writeable for your user. (NOTE: I did not test to verify this). `cap_sys_rawio` is always required.

**IMPORTANT** `setcap` modifies the extended attributes of the named file. If you delete or replace the file ---
during rebuild say --- you'll need to re-run the `setcap` command.

# Scripts
The scripts directory provides four trivial bash scripts:

* **intel_ht**: sudo run with argument `on|off`. This enables or disables Intel HW Core hyper-threading.
* **linux_nmi**: sudo run with argument `on|off`. This disables NMI interrupts recommended during PMU work. Per
Nanobench, NMI uses a counter. **Not tested or validated** 
* **linux_pmu**: sudo run with no arguments. This allows `rdpmc` assembler instruction to be called in userspace
code. It's mandatory to run this before doing PMU code.
* **linux_turbo**: sudo run with `on|off`. This enables or disables processor's turbo-mode

# Helper Programs
* `example/config.cpp`: This program pretty prints a programmable PMU configuration to stdout. Run with any argument
to emit in CSV format. The program makes the configs then prints the configs.
* `example/frequency.cpp`: This program runs a busy loop for about ~1s to estimate how many nanoseconds equals one
rdtsc cycle. Note this ratio can be calculated exactly (DPDK's `rte_get_tsc_hz()` does this), but this functionality
isn't implemented here yet. This ratio is required to convert rdtsc timer differences into conventional time units.

# Example
```
#include <intel_skylake_pmu.h>

void test1() {
  PMU pmu(PMU::ProgCounterSetConfig::k_DEFAULT_SKYLAKE_CONFIG_0);
  pmu.reset();
  pmu.start();

  // No memory accessed. volatile tells compiler
  // to not optimize out the loop into a no-op
  for (volatile int i=0; i<MAX_INTEGERS; i++);

  pmu.printSnapshot("test loop no memory accesses");
}

$ taskset -c 5 ./test.tsk
test loop no memory accesses: Intel::SkyLake CPU HW core: 5
C0  [rdtsc elapsed cycles: use with F2       ]: value: 000351306400
F0  [retired instructions                    ]: value: 000600000184, overflowed: false
F1  [no-halt cpu cycles                      ]: value: 000500483266, overflowed: false
F2  [reference no-halt cpu cycles            ]: value: 000350062518, overflowed: false
P0  [LLC references                          ]: value: 000000000064, overflowed: false
P1  [LLC misses                              ]: value: 000000000036, overflowed: false
P2  [retired branch instructions             ]: value: 000100000045, overflowed: false
P3  [retired branch instructions not taken   ]: value: 000000000004, overflowed: false
```
