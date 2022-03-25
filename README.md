# rdpmc
Task: minimum, complete example demonstrating programming Intel's PMU to count CPU level events

# Acknowledgement
This code is loosely based on [nanoBench](https://github.com/andreas-abel/nanoBench.git) however is far simpler.

# Purpose
Use this library to micro-benchmark small sections of code by programming Intel's PMU (performance monitor unit)
to count selected events e.g. LLC cache misses, instructions retired.

# Advantages
* Minimum, complete
* Does not require `yum install msr-tools`
* Works in user-space code
* Very low latency 
* Programmable event types

# Test HW
* Tested on Amazon `c5n.metal` box
* Intel(R) Xeon(R) Platinum 8124M CPU @ 3.00GHz
* CPU Family 6 (Skylake) Model 85
* `cat /sys/devices/cpu/caps/pmu_name` -> Skylake
* Last tested 2022-03-25

# Concepts
The test HW comes with four programmable counters. Each counter can be initialized to count an event type. Event types
consist of a 2-byte-tuple: (UMask, Event-Select). The [tuple values are described in Intel 64 and IA-32 Architectures Software Developer’s Manual: Volume 3](https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-system-programming-manual-325384.html) chapter 18 and 19.
For example this code programs the four counters to count four events one each:

```
+---------------------+-----------------+-----------------+-------------------------------+
| Counter             | UMask           | Event Select    | Description                   |
+---------------------+-----------------+-----------------+-------------------------------+
| 0                   | 0x00            | 0x3c            | Unhalted Core Cycles          |
+---------------------+-----------------+-----------------+-------------------------------+
| 1                   | 0x00            | 0xc0            | Retired Instructions          |
+---------------------+-----------------+-----------------+-------------------------------+
| 2                   | 0x4f            | 0x2e            | LLC References                |
+---------------------+-----------------+-----------------+-------------------------------+
| 3                   | 0x41            | 0x2e            | LLC Misses                    |
+---------------------+-----------------+-----------------+-------------------------------+

UMask/Event taken from Intel's table 18.2.1.2 from above link.
```

The method `rdpmc_initialize` sets up the counters and sets their initial value to 0. The last four lines:

```
  // UMask/Event anded with 0x410000
  wrmsr_on_cpu(0x186, cpu, 0x41003c);   // tell counter 0 to count Unhalted Core Cycles
  wrmsr_on_cpu(0x187, cpu, 0x4100c0);   // tell counter 1 to count Retired Instructions
  wrmsr_on_cpu(0x188, cpu, 0x414f2e);   // tell counter 2 to count LLC References
  wrmsr_on_cpu(0x189, cpu, 0x41412e);   // tell counter 3 to count LLC Misses
```

program the counters to count their respective events. To extract the current counter value run `rdpmc_readCounter`
with an counter argument `0<=n<=3`. It runs the assembler instruction `lfence` to force the CPU to finish any
instructions in progress, then runs the assembler instruction `rdpmc` (using GCC's built-in wrapper of same) to get
the current value as an unsigned 64-bit value. The difference between two counter reads is then the number of events
that occurred.

Counters are per HW core.

There are 100s of events Intel processors can track. See manual for their umask/event types. As far as I can tell
any of these events types can be programmerd into any of the four counters.

# Building
1. `git clone https://github.com/rodgarrison/rdpmc.git`
2. `cd rdpmc`
3. `mkdir build`
4. `cd build`
5. `cmake ..; make`
6. As root `echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`; see limitations
7. `sudo ./perf.tsk`; see limitations for root motivation

# Approach

1. Obtain the CPU for the code under test and pin the thread running it to that CPU. See `src/main.c`
2. Run `rdpmc_initialize` supplying it with the CPU from (1)
3. Read the counter values `rdpmc_readCounter`
4. Run the code to micro-benchmark
5. Re-read the counter values
6. Print the difference in counter values

# Limitations
1. Intel's PMU at least on the test HW can only track up to four values at once. If you need more you'll need to run
the code once for each set of four
2. You must run the code as root only because configuring the PMU does is accomplished by opening and writing to device
files `/dev/cpu/<cpu>/msr` which is root protected
3. Prior to running the code you must run `echo 2 > /sys/bus/event_source/devices/cpu/rdpmc`. This allows `rdpmc`
to run in user mode. Otherwise the code will segfault. Default value is `1` (kernel only). You only need to do this
once.
4. This code makes no attempt to store counter values in an array for later retrieval, however, that's a glorified
exercise in arrays
5. I make not attempt to decipher the MSR register values. I lifted these from Nanobench (thank you), but with effort
each of those hex addresses can be tracked to a PMU register via Intel's documentation
6. There's no effort to deal with overflow. PMU counters are 40 bits long while `rdpmc_initialize` resets the counter
to 0 at the beginning. Note Intel has support for detecting overflow

# Example Output:
The code as shipped benchmarks this code:
```
  unsigned s=0;
  for (unsigned i=0; i<2000; i++) {
    s+=1;
  }
```

Yielding this output:

```
[ec2-user@ip-172-31-0-202 build]$ sudo ./perf.tsk 
running pinned on CPU 41
write PMU MSR cpu 41 0x38d 0x0
write PMU MSR cpu 41 0x186 0x0
write PMU MSR cpu 41 0x187 0x0
write PMU MSR cpu 41 0x188 0x0
write PMU MSR cpu 41 0x189 0x0
write PMU MSR cpu 41 0x309 0x0
write PMU MSR cpu 41 0x30a 0x0
write PMU MSR cpu 41 0x30b 0x0
write PMU MSR cpu 41 0xc1 0x0
write PMU MSR cpu 41 0xc2 0x0
write PMU MSR cpu 41 0xc3 0x0
write PMU MSR cpu 41 0xc4 0x0
write PMU MSR cpu 41 0x390 0x0
write PMU MSR cpu 41 0x38f 0x70000000f
write PMU MSR cpu 41 0x186 0x41003c
write PMU MSR cpu 41 0x187 0x4100c0
write PMU MSR cpu 41 0x188 0x414f2e
write PMU MSR cpu 41 0x189 0x41412e
counter 0: 13915
counter 1: 8069
counter 2: 0
counter 3: 0
```
