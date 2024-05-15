project(GSMEngine)

set (CMAKE_CXX_STANDARD 14)
cmake_minimum_required(VERSION 3.0)


option(SIMULATOR "Simulator" OFF)
if(SIMULATOR)
  message(STATUS "build for AT simulator")
  add_definitions(-DSIMULATOR=1)
else()
  message(STATUS "build for GSM module")
  add_definitions(-DSIMULATOR=0)
endif()

add_subdirectory(GSMManager)
add_subdirectory(GSMManagerWrapper)
add_subdirectory(GSMApp)
