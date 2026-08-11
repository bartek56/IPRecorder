project(GSMApp)

# desktop app
include_directories(${CMAKE_SOURCE_DIR}/GSMManager ${SPDLOG_INCLUDE_DIR})
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} GSMManager ${GSM_DEBUG_SANITIZE_OPTIONS})
target_compile_options(${PROJECT_NAME} PRIVATE ${GSM_DEBUG_SANITIZE_OPTIONS})
