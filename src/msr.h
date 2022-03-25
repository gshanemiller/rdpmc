#pragma once

#include <sys/types.h>

int rdmsr_on_cpu(u_int32_t reg, int cpu, u_int64_t *value);
  // Return 0 if read into specified 'value' the contents of specified MSR 'reg' on specified 'cpu' and non-zero
  // otherwise.

int wrmsr_on_cpu(u_int32_t reg, int cpu, u_int64_t data);
  // Return 0 if wrote specified 'data' into specified MSR 'reg' on specified 'cpu' and non-zero otherwise
