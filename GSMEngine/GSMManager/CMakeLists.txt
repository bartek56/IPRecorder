project(GSMManager)


find_package (Threads REQUIRED)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(SANITIZE_OPTIONS "")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SANITIZE_OPTIONS "-fsanitize=address,undefined")
endif()

add_compile_options(-Wall -Wextra -Wpedantic ${SANITIZE_OPTIONS})
include_directories(${CMAKE_SOURCE_DIR}/GSMManager ${SPDLOG_INCLUDE_DIR})
# library for GSM serial
add_library(${PROJECT_NAME} ATConfig.hpp Serial.hpp Serial.cpp
    ATCommanderScheduler.hpp ATCommanderScheduler.cpp
    ATCommander.hpp ATCommander.cpp
    GSMManager.hpp GSMManager.cpp
    Utils.hpp Utils.cpp
    ATHandlers.hpp ATHandlers.cpp
    ATTypes.hpp)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} spdlog::spdlog ${SANITIZE_OPTIONS})
