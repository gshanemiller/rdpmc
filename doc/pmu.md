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
architecture with enough background to confidently generalize to Haswell, IceLake, etc. You can easily and cheaply
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

Recent Xeon CPU counters are typically 48-bit wide. Confirm: `cpuid | grep "bit width of fixed counters" | sort -u` for fixed-function counters and `cpuid | grep "bit width of counter" | sort -u` for programmable counters. Especially for fast changing counters like cycle count, 48-bits means there's some chance the counter will overflow if the elapsed test time is too long.

You need root/sudo ability. Although this is not a permanent or firm requirement, Linux defaults to protecting PMU usage and setup to root users:

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
```

Theoretically the number of LLC accesses should be zero because no memory was read/written. However, the actual reported numbers may bounce around not zero. Reading counters is efficient with the absolute minimum of overhead. However, setup (see MSR below) goes through expensive code that drags in memory accesses PMU sees. The CPU may (I have not checked the compiled code) have to do memory work for `i`.

PMU metrics **are per HW core**. Therefore it's critical the code under test remains pinned to a single HW core. You can either run `taskset` to pin the entire the entire process to one core, or run the code in a pinned thread. Clearly, PMU metrics are not helpful if the test code bounces between cores:

```
  #include <stdio.h>
  #include <sched.h>                                                                                                      
  #include <stdlib.h>

  int cpu = sched_getcpu();
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);

  // Pin caller's (or current) thread to cpu
  if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
      fprintf(stderr, "Error: Could not pin thread to core %d\n", cpu);
      return 1;
  }

  // setup PMU

  // run code under test

  // take PMU measurements
```

In closing note that if CPU core hyper-threading is turned off, you may be able to double the number of programmable counters. IR section 19.3.8 p751 reads `4 or (8 if a core not shared by two threads)` regarding programmable counters. This is not tested here. It's also worth noting some code disables hyper-threading to decrease CPU-cache-thrashing, and to increase performance. There may be some synergy here if performance is important. [Also note Nanobench](https://github.com/andreas-abel/nanoBench) has scripts putatively to disable/enable hyper-threading. It also notes NMI (non-maskable interrupt processing) uses a PMU counter. It has an option to temporarily disable NMI, while benchmarking runs. NMI is not further discussed here.

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

u_int64_t rdpmc_readProgrammableCounter(unsigned c) {
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

Intel organizes PMU programming in a fairly accessible way using MSRs (Model Specific Registers). A MSR is a
way to read or write data to the PMU for setup and status purposes. Reading the current counter value, however, always
uses `rdpmc`. Intel provides one MSR per counter to set its initial value, and several other MSRs to program the counter
or read its status. These MSR registers are per HW core since PMU counters are per HW core. The only annoyance is the IR
does not provide the MSR register values in the diagram when it gives the bit-spec. You have to hunt from them. As a fall
back to the IR [see Intel 64 and IA-32 Architectures Software Developer’s Manual Volume 4](intel_msr.pdf). A copy is
provided in this directory. This manual lays out all MSR values. You should be able to find MSR values (addresses)
there by searching for the symbolic MSR name.

There are two general strategies to setup the PMU to measure code under test:

* Pin thread/process
* Detect HW core running C test code
* Setup PMU counters and reset current value to 0 for C
* Run test code
* Read counter values
* Stop

Or:

* Pin thread/process
* Detect HW core C running test code
* Setup PMU counters and reset current value to 0 for C
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
See prerequisites: pin your code under test to a HW core before starting PMU work. Then run all PMU code in the same 
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
is set to enable a counter. Therefore writing a 0 will disable all fixed and programmable counters:

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
for bit 21 which was added. Setting 0 into MSR 186h will turn off programmable counter 0 because bit-22 (EN enable counter)
is cleared. As such the rest of the values don't matter:

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
You'll write these into the same IA32_PERFEVTSELx MSRs in step 1. See table above for the MSR address of each counter.

IR figure 19-6 p705 gives the bit-spec. Several of these bits are beyond my technical grasp; I will set those 0. But
four of the bits are important to mention:

* Bit 22 (EN) this must be set to enable to the counter
* Bit 17 (OS) usually this bit is cleared so the counter ignores any code spent in the OS. Most micro-benchmark code
doesn't run in the kernel anyway unless you happen to be a kernel-programmer
* Bit 16 (USR) this bit is typically enabled for user-space code the opposite of bit 17
* Bit 21 (ANY) if CPU hyper-threading is enabled, setting bit 21 ON allows PMU to count any contribution for any HW
thread running on the HW core. Clearing the bit with hyper-threading ON is unclear; I assume the PMU only counts the
HW thread running at the time the counter was defined. Because of this ambiguity I recommend turning HT off. You may
not know what other HW threads are doing in your core. This carries over to fixed counters. You may even double
your programmable counters. See note earlier re: nanoBench.

This leaves bits 0-7 for the `umask` and bits 8-15 for the `event-select`. IR 19.2.1.2 p699 gives so-called
architectural measures, and IR 19.3.8.1.2 p754 for more choices. All these and others can also be found in the Intel
performance compendium linked above.

Consider a candidate value `0x41412e`. The break down to be used with figure 19-6 is:

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
Skylake: 0x41412e bit break down. Read with IR p699,705.
         IA32_PERFEVTSELx MSRs
```

Writing `0x41412e` into 186h IA32_PERFEVTSEL does all of the following:

* 186h is for counter 0
* program counter 0 for LLC cache misses
* count only LLC misses in userspace code
* count only LLC misses in the HW thread running code
* start counter 0

all in one atomic step. Repeat for other counters. 

Step 1 disables counters in two places. Step 4 enables counters in one place, however, at step 4 the EN flag for each
counter is still off in the second place. Finally step 5 programs the counter with EN=1. This completes the programming
and enablement. The counter is now running.

Prior to step 5 there is no overhead leakage into the PMU. Recall each MSR write in step five goes through `pwrite` into
a system file possibily falling into the kernel. A typical step-5 four counter sequence looks like:

* Assumes steps 1-4 done
* writeMsr( MSR=186h, value=0x41412e, cpu=3); // cntr0: enable LLC cache misses (step 5 counter 0) in HW core 3
* writeMsr( MSR=187h, value=...,      cpu=3); // cntr1 (step 5 counter 1)
* writeMsr( MSR=188h, value=...,      cpu=3); // cntr2 (step 5 counter 2)
* writeMsr( MSR=189h, value=...,      cpu=3); // cntr3 (step 5 counter 3)

means, for example, counter 0 will see the MSR user-space writes for counters 1,2, and 3. Counter 1 will see user-space
MSR writes for counters 2,3 and so on. Since the OS flag is putatively off, kernel work should be excluded, if any.

# Mixed Counter: Fixed and Programmable

Fixed counter and programmable counters are othogonal. You can do one, or both with no side effects. But because some
MSRs control both kinds of counters including multiple counters at once, you'll you need to carefully set or clear 
controls bits for your situation. Note programmable counters can measure metrics that the fixed counters already do.
Because of the limited number of HW counters, don't waste a programmable counter when a fixed counter works fine.

# Fixed Counters

The [Intel compendium](https://perfmon-events.intel.com/) for Skylake identifies three fixed counters. See also IR
table 19-2 p703:

```
+-------------------+-------------------------------------------------+
| Fixed Counter     | Description (Elided)                            |
+-------------------+-------------------------------------------------+
| IA32_FIXED_CTR0   | Counts retired executed instructions            |
+-------------------+-------------------------------------------------+
| IA32_FIXED_CTR1   | Counts CPU cycles not in halt state             |
+-------------------+-------------------------------------------------+
| IA32_FIXED_CTR2   | Counts CPU reference cycles not in halt state   |
+-------------------+-------------------------------------------------+
Skylake: Fixed counter inventory
```

The full details of reference v. non-reference cycles is beyond my full grasp. However, the main point is that the
CPU can change its cycle frequency for load, energy, or other reasons as it runs. Therefore IA32_FIXED_CTR1 may be
hard to interpret if its frequence changed as test code ran. IA32_FIXED_CTR2 uses an unchanging frequency.

Step 1 is to disable all counters and write 0 into the fixed counter:

```
+----------------+-----------------------------+
| MSR            | MSR Write Value             |
+----------------+-----------------------------+
| 38fh           | 0                           |
+----------------+-----------------------------+
Skylake: Disable all fixed, programmable counters
         IA32_PERF_GLOBAL_CTRL MSR

+----------------+-----------------------------+
| MSR            | MSR Write Value             |
+----------------+-----------------------------+
| 38dh           | 0                           |
+----------------+-----------------------------+
Skylake: Disable all fixed counters (2nd disable)
         MIA32_FIXED_CTR_CTRL MSR

+----------------+-----------+-----------------+
| Counter        | MSR       | MSR Write Value |
+----------------+-----------+-----------------+
| 0              | 309h      | 0               |
+----------------+-----------+-----------------+
| 1              | 30ah      | 0               |
+----------------+-----------+-----------------+
| 2              | 30bh      | 0               |
+----------------+-----------+-----------------+
Skylake: Reset fixed counter to 0
         IA32_FIXED_CTR0/1/2 MSRs
```

Step 2 is to renable and configure the fixed counters and CPL (privledge level). We first enable at the global level:


```
+----------------+-----------------------------+
| MSR            | Ex. MSR Write Value         |
+----------------+-----------------------------+
| 38fh           | 0x700000000 (all fxd on)    |
+----------------+-----------------------------+
Skylake: Enable programmable (and/or fixed) counters
         See IR figure 19-8 p706 for bits. Several
         examples are given there. 1st enablement
         IA32_PERF_GLOBAL_CTRL MSR
```

As per IR p705 under PMU version 3 the bit-spec for the second enablement follows:

```
+----------------+-----------------------------+
| Bit            | Comment                     |
+----------------+-----------------------------+
| 0-1            | 0x00 -> disable             |
|                | 0x01 -> kernel              |
|                | 0x10 -> user space          |
|                | 0x11 -> OS and user space   |
|                | This bit turns on counter 0 |
|                | (when non-zero) and defines |
|                | whether it counts OS, USR   |
|                | code or both                |
+----------------+-----------------------------+
| 2              | Set to count ANY HW thread  |
|                | on the core for counter 0   |
+----------------+-----------------------------+
| 3              | Generate interrupt when cntr|
|                | 0 overflows                 |
+----------------+-----------------------------+
| 4-5            | Like bits 0-1 but for cntr1 |
+----------------+-----------------------------+
| 6              | Set to count ANY HW thread  |
|                | on the core for counter 1   |
+----------------+-----------------------------+
| 7              | Generate interupt when cntr |
|                | 1 increments (PMI)          |
+----------------+-----------------------------+
| 8-9            | Like bits 0-1 but for cntr2 |
+----------------+-----------------------------+
| 10             | Set to count ANY HW thread  |
|                | on the core for counter 2   |
+----------------+-----------------------------+
| 11             | Generate interupt when cntr |
|                | 2 increments (PMI)          |
+----------------+-----------------------------+
IA32_FIXED_CTR_CTRL MSR
```

Using this spec here's a typical way to enable all three counters for user-space work:

```
+----------------+-----------------------------+
| MSR            | Ex. MSR Write Value         |
+----------------+-----------------------------+
| 38dh           | 0x222 enable all cntrs      |
+----------------+-----------------------------+
Skylake: Enable all fixed counters (2nd enable)
         s.t. ANY count off and no interrrupts
         running in user-space code only
         IA32_FIXED_CTR_CTRL MSR
```
