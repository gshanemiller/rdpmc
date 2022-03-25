cmake_minimum_required(VERSION 3.16)
project(perf)

# Define location of build artifacts                                                                                    
#                                                                                                                       
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)                                                       
set(CMAKE_PDB_OUTPUT_DIRECTORY         ${CMAKE_SOURCE_DIR}/build)                                                       
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY     ${CMAKE_SOURCE_DIR}/build)                                                       
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY     ${CMAKE_SOURCE_DIR}/build)                                                       
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY     ${CMAKE_SOURCE_DIR}/build) 

#                                                                                                                       
# Set project wide compilation options                                                                                  
#                                                                                                                       
set(CMAKE_CXX_STANDARD 17)                                                                                              
set(CMAKE_CXX_STANDARD_REQUIRED True)                                                                                   
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -Werror")                                   
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-diagnostics-color")                                                        

add_subdirectory(src)
