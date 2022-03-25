cmake_minimum_required(VERSION 3.16)

set(SOURCES                                                                                                             
  msr.c
  main.c
  rdpmc.c
) 

#
# Build a test with logging ON for all severities at build time
#
set(PERF_TARGET perf.tsk)
add_executable(${PERF_TARGET} ${SOURCES})
target_include_directories(${PERF_TARGET} PUBLIC .)
target_compile_options(${PERF_TARGET} PUBLIC -g)
