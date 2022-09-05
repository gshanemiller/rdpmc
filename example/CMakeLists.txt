cmake_minimum_required(VERSION 3.16)

set(SOURCES                                                                                                             
  example.cpp
  ../src/intel_skylake_pmu.cpp
  ../src/intel_pmu_stats.cpp
) 

set(CFG_SOURCES                                                                                                             
  config.cpp
) 

set(FREQ_SOURCES                                                                                                             
  frequency.cpp
  ../src/intel_skylake_pmu.cpp
) 

#
# Build example
#
set(PERF_TARGET example.tsk)
add_executable(${PERF_TARGET} ${SOURCES})
target_include_directories(${PERF_TARGET} PUBLIC ../src)

#
# Build config
#
set(CONFIG_TARGET config.tsk)
add_executable(${CONFIG_TARGET} ${CFG_SOURCES})
target_include_directories(${CONFIG_TARGET} PUBLIC ../src)

#
# Build frequency
#
set(FREQ_TARGET frequency.tsk)
add_executable(${FREQ_TARGET} ${FREQ_SOURCES})
target_include_directories(${FREQ_TARGET} PUBLIC ../src)
