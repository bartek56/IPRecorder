project(GSMEngineTests)

find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(ATParserTests ATParserTests.cpp)
target_link_libraries(ATParserTests PRIVATE GSMManager GTest::gtest_main)

include(GoogleTest)
gtest_add_tests(TARGET ATParserTests)
