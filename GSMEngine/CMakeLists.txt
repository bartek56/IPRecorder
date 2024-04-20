project(GSMEngine)

set (CMAKE_CXX_STANDARD 14)
cmake_minimum_required(VERSION 3.0)

set(SPDLOG_INCLUDE_DIR /home/bartosz/Documents/projects/IPRecorder/GSMEngine/3rdparty/spdlog/include)

add_subdirectory(3rdparty/spdlog)
add_subdirectory(GSMManager)
add_subdirectory(GSMManagerWrapper)
add_subdirectory(GSMApp)
