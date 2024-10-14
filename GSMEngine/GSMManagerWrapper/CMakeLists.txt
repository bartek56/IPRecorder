project(GSMEngine)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# library for python wrapper
include_directories(${CMAKE_SOURCE_DIR}/GSMManager ${SPDLOG_INCLUDE_DIR})
pybind11_add_module(${PROJECT_NAME} GSMManagerWrapper.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE GSMManager)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto=auto")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto=auto")

set(Python_EXECUTABLE "/usr/bin/python3")
# print(sys.path[-1] belowe for Ubuntu
execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import sys; print(sys.path[-2])"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "python library install path: ${PYTHON_SITE_PACKAGES}")


install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
