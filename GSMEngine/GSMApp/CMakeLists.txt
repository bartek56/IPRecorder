project(GSMApp)

# desktop app
include_directories(${CMAKE_SOURCE_DIR}/GSMManager)
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} GSMManager)
