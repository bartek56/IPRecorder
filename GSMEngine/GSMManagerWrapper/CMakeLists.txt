project(GSMEngine)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

#too old cmake on IPRecorder
#on the IPRecorder libboost_python should be located in /usr/lib, not in /usr/local/lib

# library for python wrapper
include_directories(${CMAKE_SOURCE_DIR}/GSMManager ${SPDLOG_INCLUDE_DIR})

pybind11_add_module(${PROJECT_NAME} GSMManagerWrapper.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE GSMManager)
set(Python_EXECUTABLE "/usr/bin/python3")
# print(sys.path[-1] belowe for Ubuntu
execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import sys; print(sys.path[-2])"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "python library install path: ${PYTHON_SITE_PACKAGES}")

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
