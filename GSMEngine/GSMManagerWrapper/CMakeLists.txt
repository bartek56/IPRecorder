project(GSMEngine)

#set(BOOST_ROOT "/path/for/installed/boost")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost COMPONENTS python REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

find_package(Python COMPONENTS Interpreter Development)

# library for python wrapper
include_directories(${CMAKE_SOURCE_DIR}/GSMManager)
add_library(${PROJECT_NAME} SHARED GSMManagerWrapper.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES} GSMManager)


execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_paths()['purelib'])"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
