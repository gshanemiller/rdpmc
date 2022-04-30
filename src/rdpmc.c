#include <rdpmc.h>
#include <msr.h>
#include <x86intrin.h>

int rdpmc_initialize(int cpu) {
  wrmsr_on_cpu(0x38d, cpu, 0);                                                                                          
  wrmsr_on_cpu(0x186, cpu, 0);                                                                                          
  wrmsr_on_cpu(0x187, cpu, 0);                                                                                          
  wrmsr_on_cpu(0x188, cpu, 0);                                                                                          
  wrmsr_on_cpu(0x189, cpu, 0);                                                                                          
  wrmsr_on_cpu(0x309, cpu, 0);                                                                                          
  wrmsr_on_cpu(0x30a, cpu, 0);                                                                                          
  wrmsr_on_cpu(0x30b, cpu, 0);                                                                                          
  wrmsr_on_cpu(0xc1,  cpu, 0);                                                                                          
  wrmsr_on_cpu(0xc2,  cpu, 0);                                                                                          
  wrmsr_on_cpu(0xc3,  cpu, 0);                                                                                          
  wrmsr_on_cpu(0xc4,  cpu, 0);                                                                                          
  wrmsr_on_cpu(0x390, cpu, 0);                                                                                          
  wrmsr_on_cpu(0x38f, cpu, 0x70000000f);

  wrmsr_on_cpu(0x186, cpu, 0x41003c);                                                                                   
  wrmsr_on_cpu(0x187, cpu, 0x4100c0);                                                                                   
  wrmsr_on_cpu(0x188, cpu, 0x414f2e);                                                                                   
  wrmsr_on_cpu(0x189, cpu, 0x41412e);

  return 0;
}

u_int64_t rdpmc_readCounter(int cntr) {
  // Request running instructions finish before reading PMC counter
  __asm __volatile("lfence");
  return __rdpmc(cntr);
}
