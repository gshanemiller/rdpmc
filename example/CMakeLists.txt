cmake_minimum_required(VERSION 3.16)

set(SOURCES                                                                                                             
  example.cpp
  ../src/intel_xeon_pmu.cpp
  ../src/intel_pmu_stats.cpp
) 

#
# Build example
#
set(PERF_TARGET example.tsk)
add_executable(${PERF_TARGET} ${SOURCES})
target_include_directories(${PERF_TARGET} PUBLIC ../src)
