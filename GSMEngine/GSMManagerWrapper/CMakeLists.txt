project(GSMEngine)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

#too old cmake on IPRecorder
#on the IPRecorder libboost_python should be located in /usr/lib, not in /usr/local/lib
#find_package(Python COMPONENTS Interpreter)
#c++ -O3 -Wall -shared -std=c++11 -fPIC $(python3-config --includes) -I3rdparty/pybind11/include GSMManagerWrapper/GSMManagerWrapper.cpp -o example$(python3-config --extension-suffix)

# library for python wrapper
include_directories(${CMAKE_SOURCE_DIR}/GSMManager)
#include_directories(${CMAKE_SOURCE_DIR}/3rdparty/pybind)

pybind11_add_module(${PROJECT_NAME} GSMManagerWrapper.cpp)


set(Python_EXECUTABLE "/usr/bin/python3")
execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import sys; print(sys.path[-1])"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "python library install path: ${PYTHON_SITE_PACKAGES}")

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
