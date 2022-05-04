# Overview

Intel PMU (Performance Monitoring Unit) is the ultimate in low overhead micro-benchmarking. Intel Xeon Intel processors
typically come with three fixed-function H/W counters and four to eight programmable counters. Counters report CPU
performance measures per HW core like,

* cycles run
* instructions completed
* branches mispredicted
* Level 3 cache misses, hits
* etc.

Intel broadly categorizes performance measures as,

* architectural: items that have a primary effect on performance e.g. last level cache misses
* CPU: all other HW core measures
* Uncore (offcore): measures on HW componets that work in conjunction with the CPU

This document focuses on the first two categories.

# Objective

The objective of this document is to explain how to do PMU micro-benchmarking on the Intel Xeon Skylake Lake micro
architecture with enough background to confidently generalize to Haswell, Skylake, etc. You can easily and cheaply
obtain a Xeon SkyLake Xeon CPU running Linux for $0.50/hr at Equinix.com. Examples are included.

# Test Hardware

Tested on Equinix c3.small.x86 bare metal instance:

* Intel Xeon E-2278G @ 3.40Ghz
* Ubuntu Linux 20.04 LTS
* Last tested May 2022

# Prerequistes

Intel PMU does not require libraries, however, you need a C/C++ compiler:

```
apt update
apt install --yes make numactl libnuma-dev git htop libgtest-dev libgcc-10-dev gcc-10-doc cmake cmake-extras gdb gdb-doc pkgconf* cpuid
```

Use the `cpuid` utility to extract critical PMU information:

* Identify your micro-architecture: `cpuid | grep "simple synth" | sort -u`. Test hardware shows `Intel Core (unknown type) (Kaby Lake / Coffee Lake) {Skylake}, 14nm`
* Identify your PMU level: `cpuid | less` and search for `Architecture Performance Monitoring Features` then `version ID`. This is a number between 1-5. Test hardware shows `version ID: 4`
* Identify number of fixed counters: `cpuid | grep "fixed counters" | grep -v width | sort -u`. Test hardware shows `number of fixed counters    = 0x3 (3)`
* Identify number of programmable counters: `cpuid | grep "number of counters per logical processor" | sort -u`. Test hardware shows `number of counters per logical processor = 0x4 (4)`

Xeon counters are typically 48-bits. Confirm: `cpuid | grep "bit width of fixed counters" | sort -u` and for fixed-function counters and `cpuid | grep "bit width of counter" | sort -u` for programmable counters. Especially for fast changing counters like cycle count, 48-bits means there's some chance the counter will overflow if the elapsed test time is too long.

You need root/sudo ability. Although this is not a firm requirement, Linux OS defaults to protecting PMU usage and setup to root users:

* PMU usage is disabled by default except in the kernal. To enable for user level code, and to avoid SEGFAULTs otherwise you must run `echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`. This is a root protected file.
* PMU configuration writes to so-called MSR registers. This is done through a Linux system file which defaults to root only write access.

[It's helpful to have access to Intel 64 and IA-32 Architectures Software Developer’s Manual Vol 3](intel.pdf). A copy is provided in this repository. This document is called IR (Intel Reference) herein.

Intel organizes PMU programming by *Architectural Performance Monitoring Version*. Each version typically adds more features to the previous version:

* Version 1 IR section 19.2.1 p697
* Version 2 IR section 19.2.2 p701
* Version 3 IR section 19.2.3 p704
* Version 4 IR section 19.2.4 p707
* Version 5 IR section 19.2.5 p711

For programmable counters the metric is defined by an `umask` and `event select`. For example, PMU version 1 defines `Architectural Performance Event` in section 19.2.1.2 p699 including `LLC Reference: umask=4fh, event-select=2eh`. These items are available for all PMU versions. [Intel also offers a compendium of events here](https://perfmon-events.intel.com/). Fixed function counters are not so programmable; there is no `umask, event-select` configuration.

Because there's an upper bound on both counter sets --- usually around 7 total per core --- the code under test may need to be run more than once with different configurations if you want to get more than seven metrics. And due to the precision of PMU it's generally impossible to avoid leakage into measured values. This is doubly true if several measures are taken:

* Initialize & start counter 1
* Initialize & start counter 2
* etc.
* Run code under test
* Read counter 1
* Read counter 2
* etc.

If counters read cycle or instruction counts then counter setup and counter code reads will be counted amoung those cycles, instructions. As such micro-benchmarking typically run the code under test several times to average out overhead. That is, with a higher number of loops, the overhead becomes smaller per iteration. Readers should also be aware that it's generally not possible to fully control what the CPU does. Consider this pseudo-code:

```
  setup PMU to count LLC hits/misses
  unsigned s=0;
  for (unsigned i=0; i<10000000; ++i) {
    // make sure the compiler doesn't optimize loop out
    s+=1;
  }
  read PMU LLC hits/misses
  printf("got s=%u\n", s);
```

Theoretically the number of LLC accesses would be zero. However, the actual reported numbers may bound around a bit not zero.

PMU metrics **are per HW core**. Therefore it's critical the code under test remains pinned to a single HW core. You can either use UNIX utility `taskset` to pin the entire the entire process to one core, or run the code in pinned thread. Clearly, PMU metrics are not helpful if the test code bounces around to different cores:

```
  #define _GNU_SOURCE
  #include <stdio.h>
  #include <sched.h>                                                                                                      
  #include <stdlib.h>
  int cpu = sched_getcpu();

  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);

  // Pin current thread to cpu
  if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
      fprintf(stderr, "Error: Could not pin thread to core %d\n", cpu);
      return 1;
  }

  // setup PMU

  // run code under test

  // take PMU measurements
```

In closing note that if CPU core hyper-threading is turned off, you may be able to double the number of programmable counters. IR section 19.3.8 p751 reads ``4 or (8 if a core not shared by two threads)` regarding programmable counters. This is not tested here. It's also worth noting some code disables hyper-threading to decrease CPU-cache-thrashing, and to increase performance. There may be some synergy here if performance is important. [Also note Nanobench](https://github.com/andreas-abel/nanoBench) has putative scripts to disable/enable hyper-threading. It also notes NMI (non-maskable interrupt processing) uses a PMU counter. It has an option to temporarily disable NMI. NMI is not further discussed here.

# Reading Counter Value
Intel PMU counters are read through the `rdpmc` assembler instruction. Its argument will contain a counter number and a bit identifying counter as fixed or programmable. Usually `rdpmc` is immediately preceeded by `lfence` so pending instructions finish before reading the counter. A typical counter read functions follow. In each case `c` is the counter number zero-based. The upper bound for `c` is provided by `cpuid` above:

```
u_int64_t rdpmc_readFixedCounter(unsigned c) {
  u_int64_t a,d; 
  // Finish pending instructions
  __asm __volatile("lfence");
  // https://www.felixcloutier.com/x86/rdpmc
  // https://hjlebbink.github.io/x86doc/html/RDPMC.html
  // ECX register: bit 30 <- 1 (fixed counter) low order bits counter# zero based
  __asm __volatile("rdpmc" : "=a" (a), "=d" (d) : "c" ((1<<30)+c));
  // Result is written into EAX lower 32-bits and rest of bits up to counter-width in EDX
  return ((d<<32)+a;
}

u_int64_t rdpmc_readFixedCounter(unsigned c) {
  u_int64_t a,d; 
  // Finish pending instructions
  __asm __volatile("lfence");
  // https://www.felixcloutier.com/x86/rdpmc
  // https://hjlebbink.github.io/x86doc/html/RDPMC.html
  // ECX register: bit 30 <- 0 (programmable counter) low order bits counter# zero based
  __asm __volatile("rdpmc" : "=a" (a), "=d" (d) : "c" (c));
  // Result is written into EAX lower 32-bits and rest of bits up to counter-width in EDX
  return ((d<<32)+a;
}
```

There are two general strategies to measure code under test:

* Set the counters to 0
* Run test code
* Read counter values

And/or:

* Optionally, set the counters to 0
* At time t0, Read counter values
* Run test code
* At time t1, read counter values and report difference t1-t0 by counter
* Run test code
* At time t2, read counter values and report difference t2-t1 by counter
* etc.

The second approach runs additional risk of overflow.

# PMU Setup

Fortuantely much of PMU programming seems to be darn near identifical across Intel micro-architectures except for 
additional `umask, event-select` supported by later generations not available for earlier CPUs. For most of the typical
programmerable metrics `umask, event-select` are the same everywhere. But always [double check the compendium](https://perfmon-events.intel.com/) to make sure. Those web pages also define the fixed counters with a description.

It's worth noting Intel breaks out PMU by micro-architecture where differences arise as follows. This includes a lot of
details for core, uncore (offcore) PMU:

* IR 19.3.1 p712 Nehalem Microarchitecture
* IR 19.3.2 p726 Westmere Microarchitecture
* IR 19.3.3 p727 E7 Family
* IR 19.3.4 p727 Sandy Bridge Microarchitecture
* IR 19.3.5 p741 3rd Generation Intel
* IR 19.3.6 p741 4th Generation Intel
* IR 19.3.7 p750 5th Generation Intel
* IR 19.3.8 p751 6th Generation, 7th Generation and 8th Generation Intel
* IR 19.3.9 p760 10th Generation Intel
* IR 19.3.10 p764 12th Generation Intel
* Plus others in 19.4, 19.5, 19.6

It's not clear where the 9th and 11th generations went. The documentation will often reference Engish names by generation.
For example, here's how 19.3.8 starts:

```
19.3.8 6th Generation, 7th Generation and 8th Generation Intel® CoreTM Processor Performance Monitoring Facility

The 6th generation Intel® CoreTM processor is based on the Skylake microarchitecture. The 7th generation Intel® CoreTM processor is based on the Kaby Lake microarchitecture. The 8th generation Intel® CoreTM processors, 9th generation Intel® CoreTM processors, and Intel® Xeon® E processors are based on the Coffee Lake microarchitecture. For these microarchitectures, the core PMU supports architectural performance monitoring capability with version ID 4
```

Find your micro-architecture. Match `cpuid` attribute `cpu family` to the generation here. The next two subsections detail
how programmable, and fixed counters are setup for Skylake (6th generation). However, sufficient detail and context is given
so that readers may generalize to other CPUs. For all CPUs the general approach is:

* Locate MSR register to program in or initialize a counter
* Optionally, find MSR register to reset counter value to zero
* Optionally, find MSR register to get status information on counter
* Read or write the correct bit pattern to the MSR to accomplish goal

Because there's multiple PMU versions and several architectures the method to workout MSR values is:

* Find the first PMU version that mentions the subject in question
* Then see if the MSR was modified or extended in later PMU versions up to the CPU's version and/or
modified by the micro-architecture.

## MSR Read/Write

Under Linux the MSR registers are read or written per CPU HW core. Therefore you must know your HW core ahead of time.
See prerequisites: pin your code under test to a HW core before starting PMU work. The run all PMU code in the same 
thread.

The MSR register is accessed by reading or writing against the system file `/dev/cpu/<hw-core#>/msr`. This file is
generally readable by superusers only by default. Two APIs are needed:

```
// Read from specified MSR 'reg' on specified 'cpu' writing value into specified 'value'
int rdmsr_on_cpu(u_int32_t reg, int cpu, u_int64_t *value);

// Write specified 'data' to specified MSR 'reg' on specified 'cpu'
int wrmsr_on_cpu(u_int32_t reg, int cpu, u_int64_t data) {
```

See code for details. It's simple exercise of `open, pread, pwrite` with error checking involving no special PMU
knowledge.

## Programmable Counters

The first step is usually to disable the counter so it can be reset to 0 before restarting it. If not stopped, reset to
0 just means it'll immediately resume counting too soon. Each programmable counter has its own MSR set. IR 19.2.3 *Architectural
Performance Monitoring Version3* p704 says counter control MST zero starts at 186H. Counter 1 will be at 187H up to the last counter
reported by `cpuid` incrementing by one per counter. Figure 19-6 IR p705 gives the MSR bit specification. Mostof the
bits are explained earlier in IR 19.2.1.1 p698 PMU version 1. Bit 21 added in PMU version 3 is found IR 19.2.3 p705.

Skylake runs PMU version 4. Therefore we also check PMU Versions 2,3 ending at version 4 IR section 19.2.4 p707, and
IR 19.3.8 p751 to be sure the register format and/or MSR value hasn't changed It hasn't. Setting 0 into MSR 186h will turn
off programmable counter 0 because bit-22 (EN enable counter) is cleared. As such the rest of the values don't much matter:

```
+----------------+-----------+-----------------+
| Counter        | MSR       | MSR Write Value |
+----------------+-----------+-----------------+
| 0              | 186h      | 0               |
+----------------+-----------+-----------------+
| 1              | 187h      | 0               |
+----------------+-----------+-----------------+
| 2              | 188h      | 0               |
+----------------+-----------+-----------------+
| 3              | 189h      | 0               |
+----------------+-----------+-----------------+
Skylake: Disable first four programmable counters
         IA32_PERFEVTSELx MSRs
```

Second, reset the programmable counter to 0. IR 19.2.1.1 p697 *Architectural Performance Monitoring Version 1 Facilities*
opens writing `IA32_PMCx MSRs start at address 0C1H and occupy a contiguous block of MSR address space; the number of MSRs per logical processor is reported using CPUID.0AH:EAX[15:8]`. We check `cpuid` and note fixed counters do not appear in this leaf `0AH:EAX`, but programmable
counters do. Therefore writing zero to c1h sets programmable counter 0 to 0:

```
+----------------+-----------+-----------------+
| Counter        | MSR       | MSR Write Value |
+----------------+-----------+-----------------+
| 0              | c1h       | 0               |
+----------------+-----------+-----------------+
| 1              | c2h       | 0               |
+----------------+-----------+-----------------+
| 2              | c3h       | 0               |
+----------------+-----------+-----------------+
| 3              | c4h       | 0               |
+----------------+-----------+-----------------+
Skylake: Set (reset) programmable counter value to 0
         IA32_PMCx MSRs
```

Third, reset IA32_PERF_GLOBAL_OVF_CTRL MSR to clear any overflow bits.
