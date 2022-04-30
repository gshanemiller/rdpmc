#pragma once

#include <sys/types.h>                                                                                                  
                                                                                                                        
int rdpmc_initialize(int cpu);
u_int64_t rdpmc_readCounter(int counter);
