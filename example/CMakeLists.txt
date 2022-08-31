cmake_minimum_required(VERSION 3.16)

set(SOURCES                                                                                                             
  example.cpp
  ../src/intel_skylake_pmu.cpp
) 

set(CFG_SOURCES                                                                                                             
  config.cpp
) 

#
# Build example
#
set(PERF_TARGET example.tsk)
add_executable(${PERF_TARGET} ${SOURCES})
target_include_directories(${PERF_TARGET} PUBLIC ../src)
target_compile_options(${PERF_TARGET} PUBLIC -O2)

#
# Build config
#
set(CONFIG_TARGET config.tsk)
add_executable(${CONFIG_TARGET} ${CFG_SOURCES})
target_include_directories(${CONFIG_TARGET} PUBLIC ../src)
target_compile_options(${CONFIG_TARGET} PUBLIC -O2)
