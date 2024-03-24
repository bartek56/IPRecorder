project(GSMEngine)

cmake_minimum_required(VERSION 3.0)

find_package (Threads REQUIRED)

#set(BOOST_ROOT "/path/for/installed/boost")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost COMPONENTS python REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

# library for GSM serial
add_library(gsm Serial.cpp)
target_link_libraries(gsm ${CMAKE_THREAD_LIBS_INIT})

# library for python wrapper
add_library(GSMEngine SHARED rectangleWrapper.cpp)
target_link_libraries(GSMEngine PUBLIC ${Boost_LIBRARIES} gsm)

# desktop app
add_executable(GSMapp main.cpp)
target_link_libraries(GSMapp gsm)

