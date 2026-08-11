project(GSMManager)

find_package (Threads REQUIRED)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# library for GSM serial
add_library(${PROJECT_NAME} Serial.cpp ATMessageFramer.cpp ATParser.cpp ATCommanderScheduler.cpp ATCommander.cpp GSMManager.cpp Utils.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic ${GSM_SANITIZE_OPTIONS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} spdlog::spdlog)
