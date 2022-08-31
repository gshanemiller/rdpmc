// Original source code: https://github.com/rodgarrison/rdpmc
#pragma once

// Classes:
//    Intel::SkyLake::PMU: Manages 3 fixed counters and up to 8 programmable counters.
//                         See 'doc/pmu.doc' for details including refs for constants.

#include <assert.h>

#include <sys/types.h>
#include <sched.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <string>
#include <vector>

namespace Intel   {

// Credit to Nanobench for documenting this code and who copied it from Google. Me too!
// The purpose of this function is to stop the compiler from optimizing away a result
// (typically of a find/search here) so the outer loop isn't boiled down to a no-op or
// near no-op. It tells the compiler 'value' is needed elsewhere even if the compiler
// doesn't know who/where. Original code location:
//    https://github.com/google/benchmark include/benchmark/benchmark.h#L444
template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp& value) {
  asm volatile("" : "+m,r"(value) : : "memory");
}

namespace SkyLake {

class PMU {
  // ENUM
public:
  enum ProgCounterSetConfig {
    // +-----------------------------------------------------------------------------------------------+
    // | Intel Architecturally Significant Metrics, Basic Set                                          |
    // +-----------------------------------------------------------------------------------------------+
    // | Counter 0: https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.REFERENCE        |
    // | Counter 1: https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.REFERENCE        |
    // | Counter 2: https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.MISS             |
    // | Counter 3: https://perfmon-events.intel.com/ -> SkyLake -> BR_INST_RETIRED.ALL_BRANCHES_PS    |
    // +-----------------------------------------------------------------------------------------------+
    k_DEFAULT_SKYLAKE_CONFIG_0 = 0,
    k_DEFAULT_CONFIG_UNDEFINED = 1,
  };

private:
  enum Support {
    k_FIXED_COUNTERS            = 3,    // All boxes have 3 fixed counters
    k_MAX_PROG_COUNTERS_HT_ON   = 4,    // When CPU hyper threading ON  prog counters 0,1,2,3 available
    k_MAX_PROG_COUNTERS_HT_OFF  = 8,    // When CPU hyper threading OFF prog counters [0-7] available
  };


  const u_int32_t IA32_PERF_GLOBAL_STATUS = 0x38e;
  const u_int32_t IA32_PERF_GLOBAL_CTRL   = 0x38f;

  // MSR to configure programmable counter
  const u_int32_t IA32_PERFEVTSEL0      = 0x186;
  const u_int32_t IA32_PERFEVTSEL1      = 0x187;
  const u_int32_t IA32_PERFEVTSEL2      = 0x188;
  const u_int32_t IA32_PERFEVTSEL3      = 0x189;
  const u_int32_t IA32_PERFEVTSEL4      = 0x18a;
  const u_int32_t IA32_PERFEVTSEL5      = 0x18b;
  const u_int32_t IA32_PERFEVTSEL6      = 0x18c;
  const u_int32_t IA32_PERFEVTSEL7      = 0x18d;

  // Initial programmable counter values written here
  const u_int32_t IA32_PMC0             = 0xc1;
  const u_int32_t IA32_PMC1             = 0xc2;
  const u_int32_t IA32_PMC2             = 0xc3;
  const u_int32_t IA32_PMC3             = 0xc4;
  const u_int32_t IA32_PMC4             = 0xc5;
  const u_int32_t IA32_PMC5             = 0xc6;
  const u_int32_t IA32_PMC6             = 0xc7;
  const u_int32_t IA32_PMC7             = 0xc8;

  const u_int32_t IA32_PERF_GLOBAL_STATUS_RESET = 0x390;

  // Initial fixed counter values written here
  const u_int32_t IA32_FIXED_CTR0       = 0x309;
  const u_int32_t IA32_FIXED_CTR1       = 0x30a;
  const u_int32_t IA32_FIXED_CTR2       = 0x30b;

  // MSR to conifgure fixed counters
  const u_int32_t IA32_FIXED_CTR_CTRL   = 0x38d;
  const u_int64_t DEFAULT_FIXED_CONFIG  = 0x222;

  // Overflow masks for programmable counter 0, fixed counter 0
  // The others are generated by left shifting 
  const u_int64_t PMC0_OVERFLOW_MASK      = (1ull<<0);  // 'doc/intel_msr.pdf p287'                                      
  const u_int64_t FIXEDCTR0_OVERFLOW_MASK = (1ull<<32); // 'doc/intel_msr.pdf p287'                                      

  // DATA
  int       d_fid;                             // file handle for MSR read/write
  unsigned  d_cnt;                             // # programmable counters in use [1, k_PROGRAMMABLE_COUNTERS]
  u_int64_t d_fcfg;                            // configuration for all fixed counters
  u_int64_t d_pcfg[k_MAX_PROG_COUNTERS_HT_OFF];// configuration for each programmable counter in [0, d_cnt)
  
  // Pretty-print helper data
  std::vector<std::string> d_fixedMnemonic;    // Nickname for fixed counters e.g. 'F3' for counter 3
  std::vector<std::string> d_fixedDescription; // Full description e.g. 'Reference no-halt cycles'

  // Pretty-print helper data
  std::vector<std::string> d_progMnemonic;     // Nickname for programmable counters e.g. 'P3' for counter 3
  std::vector<std::string> d_progDescription;  // Full description e.g. 'LLC cache misses'

public:
  // CREATORS
  PMU() = delete;
    // Default constructor not provided

  explicit PMU(bool pin, ProgCounterSetConfig config);
    // Create a PMU object to run all Skylake fixed counters and programmable counters according to specified
    // enumerated value 'config'. If 'pin' is true the current thread (of the caller) is pinned to the current
    // core. Otherwise 'pin' false and it's assumed the current thread is already pinned. Upon return the caller
    // must run 'reset()'. The behavior is defined provided (*) the calling thread leaves this method pinned
    // to a HW core. PMU data will be incorrect if the calling code bounces around cores (*) 'config' refers to
    // counters '[0, k_MAX_PROG_COUNTERS_HT_ON)' only if CPU hyper threading is ON. Otherwise CPU hyper threading
    // is OFF and all programmable counters '[0, k_MAX_PROG_COUNTERS_HT_OFF]' are available. Note, this method does
    // not check if CPU hyper threading is enabled. Also note that this method initializes the descriptor arrays
    // to reflect 'config'. See 'print()'.

  explicit PMU(bool pin, unsigned count, u_int64_t *config, const char **progMnemonic, const char **progDescription);
    // Create a PMU object to run all Skylake fixed counters and 'count' programmable counters as specified in 
    // 'config', and described by 'progMnemonic, progDescription'. If 'pin' is true the current thread (of the caller)
    // is pinned to the current core. Otherwise 'pin' false and it's assumed the current thread is already pinned.
    // 'count>0' specifies both the total number of and index of each programmable counter for configuration e.g.
    // 'count==4' means counters '0,1,2,3' are selected for use. If count is zero, no programmable counters are used.
    // 'config[i]' must contain a valid 'IA32_PERFEVTSET' value for counter 'i'. See 'doc/pmu.md' for all details.
    // The arrays 'progMnemonic, progDescription' provide a textual description of each counter. See 'print()'. Upon
    // return the caller must run 'reset()'. The behavior is defined provided (*) the calling thread leaves this
    // method pinned to a HW core. PMU data will be incorrect if the calling code bounces around cores (*) If CPU
    // hyper threading is ON 'count<k_MAX_PROG_COUNTERS_HT_ON' else 'count<k_MAX_PROG_COUNTERS_HT_OFF' (*) if the
    // descriptor arrays 'progMnemonic, progDescription' have exactly 'count' valid pointers. Note, this method does
    // not check if CPU hyper threading is enabled.

  ~PMU();
    // Destroy this object. Note that upon return counter state is left unchanged.

  PMU(const PMU& other) = delete;
    // Copy constructor is not supported.

  // ACCESSORS
  int core() const;
    // Return the pinned HW core number (zero-based) of the caller. As documented in the constructors, the caller's
    // thread leaves the constructor pinned. This acceessor returns that HW core number.

  unsigned fixedCountersSupported() const;
    // Return the number of fixed, distinct counters Intel Skylake PMU can run concurrently. Note the value returned
    // is not computed. It is based on research only. See `doc/pmu.md` for more information.

  unsigned progHTCountersSupported() const;
    // Return the number of fixed, distinct programmable counters Intel Skylake PMU can run concurrently if CPU hyper
    // threading is ON. Note the value returned is not computed. It is based on research only. See `doc/pmu.md` for
    // more information.

  unsigned progNoHTCountersSupported() const;
    // Return the number of fixed, distinct programmable counters Intel Skylake PMU can run concurrently if CPU hyper
    // threading is OFF. Note the value returned is not computed. It is based on research only. See `doc/pmu.md` for
    // more information.

  unsigned programmableCounterDefined() const;
    // Return the number of programmable counters defined or requested at construction time e.g. a return value of
    // four means counters 0,1,2,3 are configured to run.

  u_int64_t programmableCounterValue(unsigned counter) const;
    // Return the current value of the specified programmable 'counter' on the HW core given by 'core()'. The behavior
    // is defined provided 'start()' or 'reset()' previously ran without error, and if 'counter' is in the range
    // '0<=counter<programmableCounterDefined()'

  u_int64_t fixedCounterValue(unsigned counter) const;
    // Return the current value of the specified fixed 'counter' on the HW core given by 'core()'. The behavior
    // is defined provided 'start()' or 'reset()' previously ran without error, and if 'counter' is in the range
    // '0<=counter<fixedCountersSupported()'. Note that as per the constructor documentation, all Skylake fixed
    // counters are always configured to run.

  u_int64_t fixedCounterConfiguration() const;
    // Return the current Skylake configuration for all fixed counters. See 'setFixedCounterConfiguration()'

  const std::vector<std::string>& fixedMnemonic() const;
    // Return a non-modifiable reference to an array of mnemonic names assigned by this class at construction time for
    // the fixed counters. The number of entries in the return value equals 'fixedCountersSupported()'.

  const std::vector<std::string>& fixedDescription() const;
    // Return a non-modifiable reference to an array of human readable descritions assigned by this class at
    // construction time for the fixed counters. The number of entries in the return value equals
    // 'fixedCountersSupported()'.

  const std::vector<std::string>& progMnemonic() const;
    // Return a non-modifiable reference to an array of mnemonic names assigned by this class at construction time
    // when a 'ProgCounterSetConfig' was given, or programmer supplied values. The number of entries in the return
    // value equals 'programmableCounterDefined()'.

  const std::vector<std::string>& progDescription() const;
    // Return a non-modifiable reference to an array of human readable descritions assigned by this class at
    // construction time when a 'ProgCounterSetConfig' was given, or programmer supplied values. The number of entries
    // in the return value equals 'programmableCounterDefined()'.

  // MANIPULATORS
  int start();
    // Return 0 if all fixed Skylake counters, and all defined programmable counters defined at construction time 
    // are running and non-zero otherwise. The behavior is defined provided 'reset()' previously ran without error.
    // Counters run until 'reset' is called.

  int reset();
    // Return zero if all counters requested at construction time are stopped, configured, and reset to 0. The counters
    // will not resume counting until 'start()' is called.

  void setFixedCounterConfiguration(u_int64_t cnfg);
    // Set the configuration for all SkyLake fixed counters using specified 'cnfg'. The supplied value will be used
    // the next time 'reset()' is called.

  int overflowStatus(u_int64_t *value);
    // Return 0 and write into specified 'value' the contents of the SkyLake IA32_PERF_GLOBAL_STATUS MSR on success and
    // non-zero otherwise. See IR p708 figure 19-10 for interpretation of value.

  bool overflow();
    // Return true if any fixed or programmable counter overflowed, and false otherwise.

  bool fixedCounterOverflowed(unsigned counter);
    // Return true if specified fixed 'counter' overflowed and false otherwise. The behavior is defined provided
    // 'start()' or 'reset()' previously ran without error, and if 'counter' is in the range the semi-closed internal
    // ''0<=counter<fixedCountersSupported()'.

  bool programmableCounterOverflowed(unsigned counter);
    // Return true if specified programmable 'counter' overflowed and false otherwise. The behavior is defined provided
    // 'start()' or 'reset()' previously ran without error, and if 'counter' is in the range the semi-closed internal
    // '0<=counter<programmableCounterDefined()'

  void print(const char *label);
    // Pretty print to stdout a human readable snapshot of counters defined at construction time and their current
    // values. Specified 'label' should describe context for PMU metrics. The behavior is defined provided 'reset()'
    // returned without error.

  PMU& operator=(const PMU& rhs) = delete;
    // Assignment operator not supported

private:
  // PRIVATE MANIPULATORS
  int rdmsr(u_int32_t reg, u_int64_t *value);
    // Return 0 if read into specified 'value' the contents of specified MSR 'reg' on the HW-core previously chosen
    // by 'open' and non-zero otherwise.

  int wrmsr(u_int32_t reg, u_int64_t data);
    // Return 0 if wrote specified 'data' into specified MSR 'reg' on the HW-core previously chosen by 'open' and
    // non-zero otherwise.

  int open(int cpu);
    // Return 0 if the MSR system file for specified 'cpu' was successfully opened. Class member 'd_fid' will hold
    // the file handle to it.

  int pinToHWCore();
    // Return 0 if the the current thread was pinned the current HW core and non-zero otherwise. Hence forth, 'core()'
    // will return this value.
};

// INLINE DEFINITIONS

// CREATORS
inline
PMU::PMU(bool pin, ProgCounterSetConfig config)
: d_fid(-1)
, d_cnt(0)
, d_fcfg(DEFAULT_FIXED_CONFIG)                                                                           
{
  assert(config>=0 && config<k_DEFAULT_CONFIG_UNDEFINED);

  if (pin) {
    pinToHWCore();
  }

  memset(d_pcfg, 0, sizeof(d_pcfg));

  d_fixedMnemonic.push_back("F0");
  d_fixedMnemonic.push_back("F1");
  d_fixedMnemonic.push_back("F2");

  d_fixedDescription.push_back("retired instructions");
  d_fixedDescription.push_back("no-halt cpu cycles");
  d_fixedDescription.push_back("reference no-halt cpu cycles");

  if (config==k_DEFAULT_SKYLAKE_CONFIG_0) {
    d_progMnemonic.push_back("P0");
    d_progMnemonic.push_back("P1");
    d_progMnemonic.push_back("P2");
    d_progMnemonic.push_back("P3");

    d_progDescription.push_back("LLC references");
    d_progDescription.push_back("LLC misses");
    d_progDescription.push_back("retired branch instructions");
    d_progDescription.push_back("retired branch instructions not taken");
  
    // +-----------------------------------------------------------------------------------------------+
    // | Intel Architecturally Significant Metrics, Basic Set                                          |
    // +-----------------------------------------------------------------------------------------------+
    // | Counter 0: https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.REFERENCE        |
    // | Counter 1: https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.REFERENCE        |
    // | Counter 2: https://perfmon-events.intel.com/ -> SkyLake -> LONGEST_LAT_CACHE.MISS             |
    // | Counter 3: https://perfmon-events.intel.com/ -> SkyLake -> BR_INST_RETIRED.ALL_BRANCHES_PS    |
    // +-----------------------------------------------------------------------------------------------+
    d_pcfg[0] = 0x414f2e;
    d_pcfg[1] = 0x41412e;
    d_pcfg[2] = 0x4104c4;
    d_pcfg[3] = 0x4110c4;

    // Four counters defined
    d_cnt = 4;
  }
}

inline
PMU::PMU(bool pin, unsigned count, u_int64_t *config, const char **progMnemonic, const char **progDescription)
: d_fid(-1)
, d_cnt(0)
, d_fcfg(DEFAULT_FIXED_CONFIG)                                                                           
{
  assert(count<k_MAX_PROG_COUNTERS_HT_OFF);
  assert(config);
  assert(progMnemonic);
  assert(progDescription);

  if (pin) {
    pinToHWCore();
  }

  memset(d_pcfg, 0, sizeof(d_pcfg));

  d_cnt = count;

  for (unsigned i=0; i<count; ++i) {
    if (progMnemonic[i]) {
      d_progMnemonic.push_back(progMnemonic[i]);
    } else {
      d_progMnemonic.push_back(std::string());
    }

    if (progDescription[i]) {
      d_progDescription.push_back(progDescription[i]);
    } else {
      d_progDescription.push_back(std::string());
    }

    d_pcfg[i] = config[i];
  }

  d_fixedMnemonic.push_back("F0");
  d_fixedMnemonic.push_back("F1");
  d_fixedMnemonic.push_back("F2");

  d_fixedDescription.push_back("retired instructions");
  d_fixedDescription.push_back("no-halt cpu cycles");
  d_fixedDescription.push_back("reference no-halt cpu cycles");
}

inline
PMU::~PMU() {
  if (d_fid!=-1) {
    close(d_fid);
    d_fid = -1;
  }
}

// ACCESSORS
inline
int PMU::core() const {
  return sched_getcpu();
}

inline
unsigned PMU::fixedCountersSupported() const {
  return (unsigned)k_FIXED_COUNTERS;
}

inline
unsigned PMU::progHTCountersSupported() const {
  return (unsigned)k_MAX_PROG_COUNTERS_HT_ON;
}

inline
unsigned PMU::progNoHTCountersSupported() const {
  return (unsigned)k_MAX_PROG_COUNTERS_HT_OFF;
}

inline
unsigned PMU::programmableCounterDefined() const {
  return d_cnt;
}

inline
u_int64_t PMU::programmableCounterValue(unsigned c) const {
  assert(c<programmableCounterDefined());
  u_int64_t a,d;                                                                                                        
  // Finish pending instructions                                                                                        
  __asm __volatile("lfence");                                                                                           
  // https://www.felixcloutier.com/x86/rdpmc                                                                            
  // https://hjlebbink.github.io/x86doc/html/RDPMC.html                                                                 
  // ECX register: bit 30 <- 0 (programmable cntr) w/ low order bits counter# zero based                                       
  __asm __volatile("rdpmc" : "=a" (a), "=d" (d) : "c" (c));
  // Result is written into EAX lower 32-bits and rest of bits up to counter-width in EDX                               
  return ((d<<32)|a);
}

inline
u_int64_t PMU::fixedCounterValue(unsigned c) const {
  assert(c<fixedCountersSupported());
  u_int64_t a,d;                                                                                                        
  // Finish pending instructions                                                                                        
  __asm __volatile("lfence");                                                                                           
  // https://www.felixcloutier.com/x86/rdpmc                                                                            
  // https://hjlebbink.github.io/x86doc/html/RDPMC.html                                                                 
  // ECX register: bit 30 <- 1 (fixed counter) w/ low order bits counter# zero based                                       
  __asm __volatile("rdpmc" : "=a" (a), "=d" (d) : "c" ((1<<30)+c));
  // Result is written into EAX lower 32-bits and rest of bits up to counter-width in EDX                               
  return ((d<<32)|a);
}

inline
u_int64_t PMU::fixedCounterConfiguration() const {
  return d_fcfg;
}                                                                        

inline
const std::vector<std::string>& PMU::fixedMnemonic() const {
  return d_fixedMnemonic;
}

inline
const std::vector<std::string>& PMU::fixedDescription() const {
  return d_fixedDescription;
}

inline
const std::vector<std::string>& PMU::progMnemonic() const {
  return d_progMnemonic;
}

inline
const std::vector<std::string>& PMU::progDescription() const {
  return d_progDescription;
}

// MANIPULATORS
inline
int PMU::start() {
  assert(d_fid>0);

  int rc;

  // Enable all fixed counters (2nd enablement)
  if ((rc = wrmsr(IA32_FIXED_CTR_CTRL, d_fcfg))!=0) {
    return rc;
  }

  // Enable defined programmable counters (2nd enablement)
  int msr = IA32_PERFEVTSEL0;
  for(int unsigned i = 0; i < d_cnt; ++i, ++msr) {
    if ((rc = wrmsr(msr, d_pcfg[i]))!=0) {
      return rc;
    }
  }

  return 0;
}

inline
int PMU::reset() {
  int rc;

  if (d_fid<0) {
    if ((rc = open(core()))!=0) {
      return rc;
    }
  }

  assert(d_fid>0);

  // Turn off all counters global level (1st disablement)
  if ((rc = wrmsr(IA32_PERF_GLOBAL_CTRL, 0))!=0) {
    return rc;
  }

  // Turn off all defined programmable counters (1st disablement)
  int msr = IA32_PERFEVTSEL0;
  for(int unsigned i = 0; i < d_cnt; ++i, ++msr) {
    if ((rc = wrmsr(msr, 0))!=0) {
      return rc;
    }
  }

  // Turn off all fixed counters (1st disablement)
  if ((rc = wrmsr(IA32_FIXED_CTR_CTRL, 0))!=0) {
    return rc;
  }

  // Reset to 0 programmable counter values
  msr = IA32_PMC0;
  for(int unsigned i = 0; i < d_cnt; ++i, ++msr) {
    if ((rc = wrmsr(msr, 0))!=0) {
      return rc;
    }
  }

  // Reset to 0 fixed counter values
  msr = IA32_FIXED_CTR0;
  for(int unsigned i = 0; i < k_FIXED_COUNTERS; ++i, ++msr) {
    if ((rc = wrmsr(msr, 0))!=0) {
      return rc;
    }
  }

  // Clear overflow bits
  if ((rc = wrmsr(IA32_PERF_GLOBAL_STATUS_RESET, 0))!=0) {
    return rc;
  }

  // Re-enable all fixed and defined programmable counters (first enablement)
  u_int64_t value = 0x700000000; // 'doc/pmd.md' discusses this number in detail
  for(int unsigned i = 0; i < d_cnt; ++i) {
    value |= (1<<i);
  }
  if ((rc = wrmsr(IA32_PERF_GLOBAL_CTRL, value))!=0) {
    return rc;
  }

  return 0;
}

inline
void PMU::setFixedCounterConfiguration(u_int64_t cnfg) {
  d_fcfg = cnfg; 
}

inline
int PMU::overflowStatus(u_int64_t *value) {
  assert(value);

  int rc;
  if ((rc = rdmsr(IA32_PERF_GLOBAL_STATUS, value))!=0) {
    return rc;
  }

  return 0;
}

inline
bool PMU::overflow() {
  bool flag(false);
  u_int64_t overFlowStatus;                                                                                             
  overflowStatus(&overFlowStatus);
  u_int64_t mask(FIXEDCTR0_OVERFLOW_MASK);
  for (unsigned i=0; i<fixedCountersSupported(); ++i, mask<<=1) {
    flag |= (overFlowStatus & mask);
  }
  mask = PMC0_OVERFLOW_MASK;
  for (unsigned i=0; i<programmableCounterDefined(); ++i, mask<<=1) {
    flag |= (overFlowStatus & mask);
  }
  return flag;
}

inline
bool PMU::fixedCounterOverflowed(unsigned counter) {
  assert(counter<fixedCountersSupported());
  u_int64_t overFlowStatus;                                                                                             
  overflowStatus(&overFlowStatus);
  u_int64_t mask(FIXEDCTR0_OVERFLOW_MASK);
  for (unsigned i=0; i<=counter; ++i, mask<<=1);
  return (overFlowStatus & mask);
}

inline
bool PMU::programmableCounterOverflowed(unsigned counter) {
  assert(counter<programmableCounterDefined());
  u_int64_t overFlowStatus;                                                                                             
  overflowStatus(&overFlowStatus);
  u_int64_t mask(PMC0_OVERFLOW_MASK);
  for (unsigned i=0; i<=counter; ++i, mask<<=1);
  return (overFlowStatus & mask);
}

inline
int PMU::rdmsr(u_int32_t reg, u_int64_t *value) {
  assert(d_fid>0);

  if (pread(d_fid, value, sizeof(u_int64_t), reg) != sizeof(u_int64_t)) {
    fprintf(stderr, "Error: MSR read error on register 0x%x %s\n", reg, strerror(errno));
    return errno;
  }

  // printf("rdmsr reg 0x%x val 0x%lx\n", reg, *data);

  return 0;
}

inline
int PMU::wrmsr(u_int32_t reg, u_int64_t data) {
  assert(d_fid>0);

  // printf("wrmsr reg 0x%x val 0x%lx\n", reg, data);

  if (pwrite(d_fid, &data, sizeof data, reg) != sizeof data) {
    fprintf(stderr, "Error: MSR write error on register 0x%x value 0x%lx: %s\n", reg, data, strerror(errno));
    return errno;
  }

  return 0;
}

inline
int PMU::open(int cpu) {
  assert(cpu>=0);
  assert(d_fid==-1);

  char msr_file_name[64];
  sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);

  d_fid = ::open(msr_file_name, O_RDWR);
  if (d_fid < 0) {
    fprintf(stderr, "Error: cannot open '%s': %s\n", msr_file_name, strerror(errno));                                                 
    return errno;
  }

  return 0;
}

inline
int PMU::pinToHWCore() {
  int cpu = sched_getcpu();                                                                                             
  cpu_set_t mask;                                                                                                       
  CPU_ZERO(&mask);                                                                                                      
  CPU_SET(cpu, &mask);                                                                                                  
                                                                                                                        
  // Pin caller's (current) thread to cpu                                                                            
  if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {                                                           
      fprintf(stderr, "Error: could not pin thread to core %d: %s\n", cpu, strerror(errno));                                                 
      return 1;                                                                                                         
  } 

  return 0;
}

} // namespace Skylake
} // namespace Intel
