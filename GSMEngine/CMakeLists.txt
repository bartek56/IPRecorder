project(GSMEngine)

cmake_minimum_required(VERSION 3.0)

#set(BOOST_ROOT "/path/for/installed/boost")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost COMPONENTS python REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

add_library(GSMEngine SHARED
    rectangle.cpp
    rectangleWrapper.cpp
)

target_link_libraries(GSMEngine PUBLIC ${Boost_LIBRARIES})
