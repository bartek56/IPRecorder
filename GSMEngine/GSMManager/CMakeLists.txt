project(GSMManager)

find_package (Threads REQUIRED)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_options(-Wall -Wextra -Wpedantic)
include_directories(${CMAKE_SOURCE_DIR}/GSMManager ${SPDLOG_INCLUDE_DIR})
# library for GSM serial
add_library(${PROJECT_NAME} Serial.cpp ATCommander.cpp GSMManager.cpp)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} spdlog::spdlog)
