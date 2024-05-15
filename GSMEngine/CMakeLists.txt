project(GSMEngine)

set (CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.0)


option(SIMULATOR "Simulator" OFF)
if(SIMULATOR)
  message(STATUS "build for AT simulator")
  add_definitions(-DSIMULATOR=1)
else()
  message(STATUS "build for GSM module")
  add_definitions(-DSIMULATOR=0)
endif()

set(SPDLOG_INCLUDE_DIR /home/bartosz/Documents/projects/IPRecorder/GSMEngine/3rdparty/spdlog/include)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(3rdparty/spdlog)
add_subdirectory(GSMManager)
add_subdirectory(GSMManagerWrapper)
add_subdirectory(GSMApp)
