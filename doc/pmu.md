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

Recenly Xeon CPU counters are typically 48-bit wides. Confirm: `cpuid | grep "bit width of fixed counters" | sort -u` and for fixed-function counters and `cpuid | grep "bit width of counter" | sort -u` for programmable counters. Especially for fast changing counters like cycle count, 48-bits means there's some chance the counter will overflow if the elapsed test time is too long.

You need root/sudo ability. Although this is not a permanent or firm requirement, Linux OS defaults to protecting PMU usage and setup to root users:

* PMU usage is disabled by default except in the kernal. To enable for user level code, and to avoid SEGFAULTs otherwise you must run `echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`. This is a root protected file.
* PMU configuration writes to so-called MSR registers. This is done through a Linux system file which defaults to root only write access.

[It's helpful to have access to Intel 64 and IA-32 Architectures Software Developer’s Manual Vol 3](intel_pmu.pdf). A copy is provided in this directory. This document is called IR (Intel Reference) herein.

Intel organizes PMU programming by *Architectural Performance Monitoring Version*. Each version typically extends or revises a feature from previous version. It's important to know your CPU's PMU level (version) so you know what the PMU can do and how to do it:

* Version 1 IR section 19.2.1 p697
* Version 2 IR section 19.2.2 p701
* Version 3 IR section 19.2.3 p704
* Version 4 IR section 19.2.4 p707
* Version 5 IR section 19.2.5 p711

Intel PMU provides a set number of programmable and fixed-purpose counters per core. In the programmable case the metric is defined by an `umask` and `event select`. For example, PMU version 1 defines `Architectural Performance Event` in section 19.2.1.2 p699 including `LLC Reference: umask=4fh, event-select=2eh`. These items are available for all PMU versions. [Intel provides a compendium of events here](https://perfmon-events.intel.com/). Fixed function counters are not programmable; there is no `umask, event-select` configuration.

Because there's an upper bound on both counter sets --- usually around 7 total per core --- the code under test may need to run more than once with different configurations if you want to get more than seven metrics. And due to the precision of PMU, it's generally impossible to avoid leaking PMU setup into measured values. This is doubly true if several measures are taken:

* Initialize & start counter 1
* Initialize & start counter 2
* etc.
* Run code under test
* Read counter 1
* Read counter 2
* etc.

If counters read cycle or instruction counts then counter setup and counter code reads will be counted amoung those cycles, instructions. As such micro-benchmarking typically run the code under test several times to average out overhead. Setup uses a fixed, constant number of resources. The more the test code is run the smaller the setup becomes as a fraction of the total numbers measureed.

It's generally not possible to fully control what the CPU does and what PMU sees. Consider this pseudo-code:

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

Theoretically the number of LLC accesses should be zero. However, the actual reported numbers may bounce around not zero. Reading counters is efficient with the absolute minimum of overhead. However, setup (see MSR below) goes through expensive code that drags in memory accesses PMU sees. 

PMU metrics **are per HW core**. Therefore it's critical the code under test remains pinned to a single HW core. You can either run `taskset` to pin the entire the entire process to one core, or run the code in a pinned thread. Clearly, PMU metrics are not helpful if the test code bounces between cores:

```
  #include <stdio.h>
  #include <sched.h>                                                                                                      
  #include <stdlib.h>
  int cpu = sched_getcpu();

  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);

  // Pin caller's (current) thread to cpu
  if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
      fprintf(stderr, "Error: Could not pin thread to core %d\n", cpu);
      return 1;
  }

  // setup PMU

  // run code under test

  // take PMU measurements
```

In closing note that if CPU core hyper-threading is turned off, you may be able to double the number of programmable counters. IR section 19.3.8 p751 reads ``4 or (8 if a core not shared by two threads)` regarding programmable counters. This is not tested here. It's also worth noting some code disables hyper-threading to decrease CPU-cache-thrashing, and to increase performance. There may be some synergy here if performance is important. [Also note Nanobench](https://github.com/andreas-abel/nanoBench) has scripts putatively to disable/enable hyper-threading. It also notes NMI (non-maskable interrupt processing) uses a PMU counter. It has an option to temporarily disable NMI, while benchmarking runs. NMI is not further discussed here.

# Reading Counter Value

Intel PMU counters are read through the `rdpmc` assembler instruction. Its argument will contain a counter number and a bit identifying counter as fixed or programmable. Usually `rdpmc` is immediately preceeded by `lfence` so pending instructions finish before reading the counter. Typical counter read functions follow. In each case `c` is the counter number zero-based. The upper bound for `c` is provided by `cpuid` above:

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
  return ((d<<32)|a;
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
  return ((d<<32)|a;
}
```

# PMU Programming Orientation

Fortuantely PMU programming seems to be darn near identifical across most Intel micro-architectures except for additional
`umask, event-select` supported by later generations not available for earlier CPUs. The most important programmable
metrics were introduced early. And their `umask, event-select` numbers haven't changed for the most part. But always
[double check the compendium](https://perfmon-events.intel.com/) to make sure. The web site pages also define the fixed
counters with a description. You'll need to know your CPUs micro-architecture name (Skylake) ahead of time to use it.

Intel breaks out PMU by micro-architecture where differences arise as follows. This includes a lot of details for core,
uncore (offcore) PMU:

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

The documentation will reference generation nicknames. For example, 19.3.8 starts:

```
19.3.8 6th Generation, 7th Generation and 8th Generation Intel® CoreTM Processor Performance Monitoring Facility

The 6th generation Intel® CoreTM processor is based on the Skylake microarchitecture. The 7th generation Intel® CoreTM processor is based on the Kaby Lake microarchitecture. The 8th generation Intel® CoreTM processors, 9th generation Intel® CoreTM processors, and Intel® Xeon® E processors are based on the Coffee Lake microarchitecture. For these microarchitectures, the core PMU supports architectural performance monitoring capability with version ID 4
```

Find your micro-architecture. Because there's multiple PMU versions and several micro architectures the method to workout
values for your test hardware is:

* Find the PMU version that first mentions the issue/subject in question
* Then see if the subsequent PMU versions modify or extend issue up until the PMU version of your CPU
* Then checkout the documentation for your micro-architecture to see if there's any other modifications

Intel organizes PMU programming in a fairly accessible way using MSRs (Model Specific Registers). A MSR register is a
way to read or write data to the PMU for setup and status purposes. Reading the current counter value, however, always
uses `rdpmc`. Intel provides one MSR per counter to set its initial value, and several other MSRs to program the counter
or read its status. These MSR registers are per HW core since PMU counters are per HW core. The only annoyance is the IR
does not provide the MSR register values in the diagram which gives its bit-spec. You have to hunt from them. As a fall
back to the IR [see Intel 64 and IA-32 Architectures Software Developer’s Manual Volume 4](intel_msr.pdf). A copy is
provided in this directory. You should be able to find MSR values (addresses) there. 

There are two general strategies to setup the PMU to measure code under test:

* Pin thread/process
* Detect HW core running test code
* Setup PMU counters and reset current value to 0
* Run test code
* Read counter values
* Stop

And/or:

* Pin thread/process
* Detect HW core running test code
* Setup PMU counters and reset current value to 0
* Run test code
* At time t0, read counter values
* Run test code
* At time t1, read counter values and report difference t1-t0 by counter
* Run test code
* At time t2, read counter values and report difference t2-t1 by counter
* etc.

The second approach runs additional risk of overflow. Overflow is a status that can be detected through a MSR read. It
happens when the real value uses more bits than the PMU counter supports often 40 to 48 bits. See `cpuid` above.

# MSR Read/Write

Under Linux the MSR registers are read or written per CPU HW core. Therefore you must know your HW core ahead of time.
See prerequisites: pin your code under test to a HW core before starting PMU work. The run all PMU code in the same 
thread.

A MSR register is accessed by reading or writing against the system file `/dev/cpu/<hw-core#>/msr`. This file is
readable by superusers only by default. Two APIs are needed:

```
// Read from specified MSR 'reg' on specified 'cpu' writing value into specified 'value'
int rdmsr_on_cpu(u_int32_t reg, int cpu, u_int64_t *value);

// Write specified 'data' to specified MSR 'reg' on specified 'cpu'
int wrmsr_on_cpu(u_int32_t reg, int cpu, u_int64_t data) {
```

See code for details. It's a simple exercise of `open, pread, pwrite` with error checking involving no special PMU
knowledge.

# Programmable Counters

In this section a programmable PMU counter:

* is disabled
* current value reset to 0
* overflow and other statuses cleared
* programmed by `umask, event-select` and started

The documentation is specific to Skylake, however, it's detailed enough that readers can generalize to other CPUs. And
in fact, Coffee/Kabby/Skylake pretty much all work the same.

The first step is usually to disable the counter so it can be reset to 0 before restarting it. If not stopped, reset to
0 just means it'll immediately resume counting too soon. We want the counter the start from 0 delaying the start or 
enablement of counting to just before the code under test runs. IR 19.2.2 p701 PMU version 2 provides the IA32_PERF_GLOBAL_CTRL
MSR for this purpose. This is given again in PMU version 3 IR p706 and not modified further in PMU version 4. This 
MSR controls enablement/disablement for all fixed and programmable counters. Therefore a single MSR write to this MSR
will accomplish our goal. To find the [MSR address consult the MSR manual](intel_msr.pdf). IR p706 indicates that a bit
should be set to enable one of the counters. Therefore writing a 0 will disable all fixed and programmable counters:

```
+----------------+-----------------------------+
| MSR            | MSR Write Value             |
+----------------+-----------------------------+
| 38fh           | 0                           |
+----------------+-----------------------------+
Skylake: Disable all fixed, programmable counters
         IA32_PERF_GLOBAL_CTRL MSR
```

Each programmable counter has its own MSR set. IR 19.2.3 *Architectural Performance Monitoring Version3* p704 says
programmable counter zero starts at 186H. Counter 1 will be at 187H up to the last counter reported by `cpuid` incrementing
by one per counter. Figure 19-6 IR p705 gives the MSR bit specification. Most of the bits are explained earlier in IR
19.2.1.1 p698 PMU version 1. Bit 21 added in PMU version 3 is found IR 19.2.3 p705. Skylake runs PMU version 4. Therefore
we also check PMU Version version 4 IR section be sure the register format and/or MSR value hasn't changed It hasn't expect
for bit 21. Setting 0 into MSR 186h will turn off programmable counter 0 because bit-22 (EN enable counter) is cleared.
As such the rest of the values don't matter:

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
Skylake: Disable (cont) first four programmable counters
         IA32_PERFEVTSELx MSRs
```

Once again recall writing to a MSR register requires the HW core to which the MSR register applies. The HW core
number is not in the table. It's implied. PMU counters are always per core.

**Note:** I am not clear if disabling the counters via IA32_PERF_GLOBAL_CTRL disables/clears the EN bit (assuming it was
set) in the IA32_PERFEVTSELx MSRs. In fact, the MSR writes to 186-189h may not be required. This needs to be tested.

Second, reset the programmable counter to 0. IR 19.2.1.1 p697 *Architectural Performance Monitoring Version 1 Facilities*
opens writing `IA32_PMCx MSRs start at address 0C1H and occupy a contiguous block of MSR address space; the number of MSRs
per logical processor is reported using CPUID.0AH:EAX[15:8]`. Check `cpuid` and note fixed counters **do not appear** in
`cpuid` leaf `0AH:EAX`, but programmable counters do. Therefore writing zero to c1h sets programmable counter 0 to 0:

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

Third, reset the overflow indicator bits. This completes the reset to 0. IR 19.2.4 p707 PMU version 4 reorganizes how this
is done. It replaces `IA32_PERF_GLOBAL_OVF_CTRL` MSR from earlier PMU versions with `IA32_PERF_GLOBAL_STATUS_RESET` to clear
status bits. `IA32_PERF_GLOBAL_STATUS_SET` MSR sets status bits. `IA32_PERF_GLOBAL_INUSE` MSR is used to see which counters
are in use. These seem to be write-only MSRs. `IA32_PERF_GLOBAL_STATUS` reports status bits on MSR read. For our purposes
only `IA32_PERF_GLOBAL_STATUS_RESET` is required here. I was not able to locate the MSR register values for these MSRs.
The IR does not give them explicitly. However, [it can be quickly found here](intel_msr.pdf) by searching for
`IA32_PERF_GLOBAL_STATUS_RESET`. We can write a value of `0` into 390h to clear everything:

```
+----------------+-----------------------------+
| MSR            | MSR Write Value             |
+----------------+-----------------------------+
| 390h           | 0                           |
+----------------+-----------------------------+
Skylake: Clear overlow (and all other) statuses
         IA32_PERF_GLOBAL_STATUS_RESET MSR
```

Next, reset IA32_PERF_GLOBAL_CTRL to enable counters desired. IR figure 19-8 p706 and the [MSR manual](intel_msr.pdf) p45
give the bit-spec for this register. Bit 0 enables programmable counter 0; bit 1 for counter 1 etc. And 32-34 are for the
fixed counters. So for example writing `0x70000000F` would enable all three fixed counters, and the first four programmable
counters. Writing `0x00000000F` disables the fixed counters enabling the first four programmable counters. Programmable
counter 1 can be enabled with `0x01`.

```
+----------------+-----------------------------+
| MSR            | Ex. MSR Write Value         |
+----------------+-----------------------------+
| 38fh           | 0x70000000F (all ctrs on)   |
+----------------+-----------------------------+
| 38fh           | 0x000000001 (prog cntr1)    |
+----------------+-----------------------------+
| 38fh           | 0x00000000F (all prg cntrs) |
+----------------+-----------------------------+
Skylake: Enable programmable (and/or fixed) counters
         See IR figure 19-8 p706 for bits. Several
         examples are given here
         IA32_PERF_GLOBAL_CTRL MSR
```

Finally, program the counter by `umask, event-select`. In addition to those settings, other bits are set as a combined
value including the enable indicator bit. In this way, the counter can be defined and started in one MSR write. Once
this write is done then, together with the MSR write completed immediately prior, the counter is enabled and running.

IR figure 19-6 p705 gives the bit-spec. Several of these bits are beyond my technical grasp; I will set those 0. But
four of the bits are important to mention:

* Bit 22 (EN) this must be set to enable to the counter
* Bit 17 (OS) usually this bit is cleared so the counter ignores any code spent in the OS. Most micro-benchmark code
doesn't run in the kernel anyway unless you happen to be a kernel-programmer
* Bit 16 (USR) this bit is typically enabled for the opposite reason of bit 17
* Bit 21 (ANY) if CPU hyper-threading is enabled, setting bit 21 ON allows PMU to count any contribution for any HW
thread running on the HW core. Clearing the bit with hyper-threading ON is unclear; I assume the PMU only counts the
HW thread running at the time the counter was defined. Because of this ambiguity I recommend turning HT off

This leaves bits 0-7 for the `umask` and bits 8-15 for the `event-select`. See IR 19.2.1.2 p699 gives so-called
architectural measures, or see IR 19.3.8.1.2 p754 for choices. All of these and others can also be found in the Intel
performance compendium linked above.

Consider a candidate value `0x41412e`. The break down to be used figure 19-6 is:

```
+------------+----------+------------+-------------------------------+
| Bits       | Value    | Symbol     | Comment                       |
+------------+----------+------------+-------------------------------+
| 0-7        | 2eh      | Event      | LLC Misses (e.g. IR p699)     |
+------------+----------+------------+-------------------------------+
| 8-15       | 41h      | Umask      | LLC Misses (e.g. IR p699)     |
+------------+----------+------------+-------------------------------+
| 16         | 1        | USR        | Count user space code         |
+------------+----------+------------+-------------------------------+
| 22         | 1        | EN         | Enable the counter            |
+------------+----------+------------+-------------------------------+
Skylake: 0x41412e bit break down. Read with IR p699,705
         IA32_PERFEVTSELx MSRs
```

# Fixed Counters
