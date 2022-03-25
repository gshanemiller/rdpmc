#pragma once

#include <sys/types.h>                                                                                                  
                                                                                                                        
int rdpmc_initialize(int cpu);
int rdpmc_snapshot();
int rdpmc_close();

u_int64_t rdpmc_readCounter(int counter);
