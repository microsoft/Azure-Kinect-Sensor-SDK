add_executable(dynlib_ut dynlib.cpp)

target_link_libraries(dynlib_ut PRIVATE
    gtest::gtest
    k4ainternal::dynlib
    k4ainternal::utcommon)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # Search the same directory as the exectuable for the shared object
    target_link_libraries(dynlib_ut PRIVATE "-Wl,-rpath,'$$ORIGIN'")
endif()

k4a_add_tests(TARGET dynlib_ut TEST_TYPE UNIT)
