cmake_minimum_required(VERSION 3.16)

set(SOURCES                                                                                                             
  intel_skylake_pmu.cpp
  intel_pmu_stats.cpp
) 

#
# Build code into library to verify builds
#
set(PERF_TARGET pmc)
add_library(${PERF_TARGET} STATIC ${SOURCES})
target_include_directories(${PERF_TARGET} PUBLIC .)
