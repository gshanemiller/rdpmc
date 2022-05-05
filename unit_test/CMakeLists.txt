cmake_minimum_required(VERSION 3.16)

set(TEST_TARGET unit_test.tsk)

set(SOURCES                                                                                                             
  ../src/intel_skylake_pmu.cpp
  unit_test.cpp
) 

add_executable(${TEST_TARGET} ${SOURCES})

target_include_directories(${TEST_TARGET} PUBLIC ../src)
target_link_libraries(${TEST_TARGET} PUBLIC pthread)
target_link_libraries(${TEST_TARGET} PUBLIC gtest)

add_test(
    NAME    ${TEST_TARGET}
    COMMAND "<TARGET_FILE:${TEST_TARGET}>"
)
