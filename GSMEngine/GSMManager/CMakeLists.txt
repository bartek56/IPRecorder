project(GSMManager)

find_package (Threads REQUIRED)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# library for GSM serial
add_library(${PROJECT_NAME} Serial.cpp ATCommander.cpp GSMTasks.cpp GSMManager.cpp)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
