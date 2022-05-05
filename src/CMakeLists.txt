cmake_minimum_required(VERSION 3.16)

set(SOURCES                                                                                                             
  intel_skylake_pmu.cpp
) 

#
# Build code into library to verify builds
#
set(PERF_TARGET pmc)
add_library(${PERF_TARGET} STATIC ${SOURCES})
target_include_directories(${PERF_TARGET} PUBLIC .)
target_compile_options(${PERF_TARGET} PUBLIC -O2 -g)
