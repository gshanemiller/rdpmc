cmake_minimum_required(VERSION 3.16)
project(perf)

#                                                                                                                       
# Set project wide compilation options                                                                                  
#                                                                                                                       
set(CMAKE_CXX_STANDARD 17)                                                                                              
set(CMAKE_CXX_STANDARD_REQUIRED True)                                                                                   
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -Werror")                                   
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-diagnostics-color")                                                        

add_subdirectory(src)

enable_testing()                                                                                                      
add_subdirectory(unit_test)

add_subdirectory(example)
