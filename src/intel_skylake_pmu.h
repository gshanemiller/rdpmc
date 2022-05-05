#pragma once

// Classes:
//    Intel::SkyLake::PMU: Manages Skylake's 3 fixed counters and up to 4 programmable counters.
//                         This is a straightforward implementation of 'doc/pmu.md' for Skylake.

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

namespace Intel   {
namespace SkyLake {

class PMU {
  // ENUM
  enum Support {
    k_FIXED_COUNTERS        = 3,
    k_PROGRAMMABLE_COUNTERS = 4
  };

  // CONST DATA
  const u_int32_t IA32_PERF_GLOBAL_CTRL = 0x38f;

  const u_int32_t IA32_PERFEVTSEL0      = 0x186;
  const u_int32_t IA32_PERFEVTSEL1      = 0x187;
  const u_int32_t IA32_PERFEVTSEL2      = 0x188;
  const u_int32_t IA32_PERFEVTSEL3      = 0x189;
  const u_int32_t IA32_PMC0             = 0xc1;
  const u_int32_t IA32_PMC1             = 0xc2;
  const u_int32_t IA32_PMC2             = 0xc3;
  const u_int32_t IA32_PMC3             = 0xc4;

  const u_int32_t IA32_PERF_GLOBAL_STATUS_RESET = 0x390;

  const u_int32_t IA32_FIXED_CTR_CTRL   = 0x38d;
  const u_int32_t IA32_FIXED_CTR0       = 0x309;
  const u_int32_t IA32_FIXED_CTR1       = 0x30a;
  const u_int32_t IA32_FIXED_CTR2       = 0x30b;
  const u_int64_t DEFAULT_FIXED_CONFIG  = 0x222;

  // DATA
  int       d_fid;                            // file handle for MSR read/write
  unsigned  d_cnt;                            // # programmable counters in use [1, k_PROGRAMMABLE_COUNTERS]
  u_int64_t d_fcfg;                           // configuration for all fixed counters
  u_int64_t d_pcfg[k_PROGRAMMABLE_COUNTERS];  // configuration for each programmable counter

public:
  // CREATORS
  explicit PMU(bool pin, u_int64_t cntr0Cfg);
    // Create a PMU object to configure and run all Skylake fixed counters, and programmable counter 0 only using
    // specified 'cntr0Cfg' for counter zero's IA32_PERFEVTSEL. See `doc/pmu.md` for how to define cntr0Cfg. If 'pin'
    // is true, the current thread (of the caller) is pinned to the current core. Otherwise 'pin' false and it's assumed
    // the current thread is already pinned. The behavior is defined provided 'cntr0Cfg!=0'. Upon return the caller
    // must run 'reset()' before invoking 'start()' or reading a counter value. Note the fixed counters are configured
    // to count in userspace only, and will not generate an interrupt on overflow.

  explicit PMU(bool pin, u_int64_t cntr0Cfg, u_int64_t cntr1Cfg);
    // Create a PMU object to configure and run all Skylake fixed counters, and programmable counters 0,1 only using
    // specified 'cntr0Cfg, cntr1Cfg'. See constructor above for other critical details. The behavior is defined if
    // config arguments are non-zero.

  explicit PMU(bool pin, u_int64_t cntr0Cfg, u_int64_t cntr1Cfg, u_int64_t cntr2Cfg);
    // Create a PMU object to configure and run all Skylake fixed counters, and programmable counters 0,1,2 only using
    // specified 'cntr0Cfg, cntr1Cfg, cntr2Cfg'. See constructor above for other critical details. The behavior is
    // defined if config arguments are non-zero.                                                     

  explicit PMU(bool pin, u_int64_t cntr0Cfg, u_int64_t cntr1Cfg, u_int64_t cntr2Cfg, u_int64_t cntr3Cfg);
    // Create a PMU object to configure and run all Skylake fixed counters, and programmable counters 0,1,2,3 using
    // specified 'cntr0Cfg, cntr1Cfg, cntr2Cfg, cntr3Cfg'. See constructor above for other critical details. The
    // behavior is defined if config arguments are non-zero.                                                     

  ~PMU();
    // Destroy this object. Note that upon return counter state is left unchanged.

  PMU(const PMU& other) = delete;
    // Copy constructor is not supported.

  // ACCESSORS
  int core() const;
    // Return the pinned HW core number (zero-based) of the caller. As documented in the constructors, the caller's
    // thread leaves the constructor pinned.

  int fixedCountersSupported() const;
    // Return the number of fixed counters Intel Skylake PMU supports. Note the value returned is not computed. It
    // is based on research only. See `doc/pmu.md` for more information.

  int programmableCounterSupported() const;
    // Return the number of programmable counters Intel Skylake PMU supports. Note the value returned is not computed.
    // It is based on research only. See `doc/pmu.md` for more information.

  int programmableCounterDefined() const;
    // Return the number of programmable counters defined or requested at construction time.

  u_int64_t programmableCounterValue(unsigned counter) const;
    // Return the current value of the specified programmable 'counter' on the HW core given by 'core()'. The behavior
    // is defined provided 'start()' or 'reset()' previously ran without error, and if 'counter' is in the range
    // '0<=counter<programmableCounterDefined()'

  u_int64_t fixedCounterValue(unsigned counter) const;
    // Return the current value of the specified fixed 'counter' on the HW core given by 'core()'. The behavior
    // is defined provided 'start()' or 'reset()' previously ran without error, and if 'counter' is in the range
    // '0<=counter<fixedCountersSupported()'. Note that as per the constructor documentation, all Skylake fixed
    // counters are always configured to run.

  // MANIPULATORS
  int start();
    // Return 0 if all fixed Skylake counters, and all defined programmable counters defined at construction time 
    // are running and non-zero otherwise. The behavior is defined provided 'reset()' previously ran without error.
    // Counters run until 'reset' is called.

  int reset();
    // Return zero if all counters setup at construction time are stopped and reset to 0. The counters will not resume
    // counting until 'start()' is called.

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
PMU::PMU(bool pin, u_int64_t cntr0Cfg)
: d_fid(-1)
, d_cnt(1)
, d_fcfg(DEFAULT_FIXED_CONFIG)                                                                           
{
  assert(cntr0Cfg!=0);

  d_pcfg[0] = cntr0Cfg;
  d_pcfg[1] = 0;
  d_pcfg[2] = 0;
  d_pcfg[3] = 0;
  
  if (pin) {
    pinToHWCore();
  }
}

inline
PMU::PMU(bool pin, u_int64_t cntr0Cfg, u_int64_t cntr1Cfg)
: d_fid(-1)
, d_cnt(2)
, d_fcfg(DEFAULT_FIXED_CONFIG)                                                                           
{
  assert(cntr0Cfg!=0);
  assert(cntr1Cfg!=0);

  d_pcfg[0] = cntr0Cfg;
  d_pcfg[1] = cntr1Cfg;
  d_pcfg[2] = 0;
  d_pcfg[3] = 0;
  
  if (pin) {
    pinToHWCore();
  }
}

inline
PMU::PMU(bool pin, u_int64_t cntr0Cfg, u_int64_t cntr1Cfg, u_int64_t cntr2Cfg)
: d_fid(-1)
, d_cnt(3)
, d_fcfg(DEFAULT_FIXED_CONFIG)                                                                           
{
  assert(cntr0Cfg!=0);
  assert(cntr1Cfg!=0);
  assert(cntr2Cfg!=0);

  d_pcfg[0] = cntr0Cfg;
  d_pcfg[1] = cntr1Cfg;
  d_pcfg[2] = cntr2Cfg;
  d_pcfg[3] = 0;
  
  if (pin) {
    pinToHWCore();
  }
}

inline
PMU::PMU(bool pin, u_int64_t cntr0Cfg, u_int64_t cntr1Cfg, u_int64_t cntr2Cfg, u_int64_t cntr3Cfg)
: d_fid(-1)
, d_cnt(4)
, d_fcfg(DEFAULT_FIXED_CONFIG)                                                                           
{
  assert(cntr0Cfg!=0);
  assert(cntr1Cfg!=0);
  assert(cntr2Cfg!=0);
  assert(cntr3Cfg!=0);

  d_pcfg[0] = cntr0Cfg;
  d_pcfg[1] = cntr1Cfg;
  d_pcfg[2] = cntr2Cfg;
  d_pcfg[3] = cntr3Cfg;
  
  if (pin) {
    pinToHWCore();
  }
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
int PMU::fixedCountersSupported() const {
  return (int)(k_FIXED_COUNTERS);
}

inline
int PMU::programmableCounterSupported() const {
  return int(k_PROGRAMMABLE_COUNTERS);
}

inline
int PMU::programmableCounterDefined() const {
  return int(d_cnt);
}

inline
u_int64_t PMU::programmableCounterValue(unsigned c) const {
  u_int64_t a,d;                                                                                                        
  // Finish pending instructions                                                                                        
  __asm __volatile("lfence");                                                                                           
  // https://www.felixcloutier.com/x86/rdpmc                                                                            
  // https://hjlebbink.github.io/x86doc/html/RDPMC.html                                                                 
  // ECX register: bit 30 <- 1 (fixed counter) low order bits counter# zero based                                       
  __asm __volatile("rdpmc" : "=a" (a), "=d" (d) : "c" ((1<<30)+c));                                                     
  // Result is written into EAX lower 32-bits and rest of bits up to counter-width in EDX                               
  return ((d<<32)|a);
}

inline
u_int64_t PMU::fixedCounterValue(unsigned c) const {
  u_int64_t a,d;                                                                                                        
  // Finish pending instructions                                                                                        
  __asm __volatile("lfence");                                                                                           
  // https://www.felixcloutier.com/x86/rdpmc                                                                            
  // https://hjlebbink.github.io/x86doc/html/RDPMC.html                                                                 
  // ECX register: bit 30 <- 1 (fixed counter) low order bits counter# zero based                                       
  __asm __volatile("rdpmc" : "=a" (a), "=d" (d) : "c" ((1<<30)+c));                                                     
  // Result is written into EAX lower 32-bits and rest of bits up to counter-width in EDX                               
  return ((d<<32)|a);
}

inline
int PMU::start() {
  assert(d_fid>0);

  int rc;

  // Enable all fixed counters (2nd enablement)
  if ((rc = wrmsr(IA32_PERF_GLOBAL_CTRL, d_fcfg))!=0) {
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

  // Turn off all counters global level
  if ((rc = wrmsr(IA32_PERF_GLOBAL_CTRL, 0))!=0) {
    return rc;
  }

  // Turn off all defined programmable counters
  int msr = IA32_PERFEVTSEL0;
  for(int unsigned i = 0; i < d_cnt; ++i, ++msr) {
    if ((rc = wrmsr(msr, 0))!=0) {
      return rc;
    }
  }

  // Turn off all fixed counters
  if ((rc = wrmsr(IA32_FIXED_CTR_CTRL, 0))!=0) {
    return rc;
  }

  // Reset to 0 programmable counters
  msr = IA32_PMC0;
  for(int unsigned i = 0; i < d_cnt; ++i, ++msr) {
    if ((rc = wrmsr(msr, 0))!=0) {
      return rc;
    }
  }

  // Reset to 0 fixed counters
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
  u_int64_t value = 0x700000000;
  for(int unsigned i; i < d_cnt; ++i, ++msr) {
    value |= (1<<i);
  }
  if ((rc = wrmsr(IA32_PERF_GLOBAL_CTRL, 0))!=0) {
    return rc;
  }

  return 0;
}

inline
int PMU::rdmsr(u_int32_t reg, u_int64_t *value) {
  assert(d_fid>0);

  if (pread(d_fid, value, sizeof(u_int64_t), reg) != sizeof(u_int64_t)) {
    fprintf(stderr, "Error: MSR read error on register 0x%x %s\n", reg, strerror(errno));
    return errno;
  }

  return 0;
}

inline
int PMU::wrmsr(u_int32_t reg, u_int64_t data) {
  assert(d_fid>0);

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
