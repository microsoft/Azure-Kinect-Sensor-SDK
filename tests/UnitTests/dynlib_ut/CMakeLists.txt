# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(dynlib_ut dynlib.cpp)

set(TEST_LIBRARY_NAME "test_dynlib")
set(TEST_LIBRARY_MAJOR_VER 1)
set(TEST_LIBRARY_MINOR_VER 2)

add_library(${TEST_LIBRARY_NAME} SHARED testdynlib.c)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

    set_target_properties(
        ${TEST_LIBRARY_NAME}
        PROPERTIES
            OUTPUT_NAME
                "${TEST_LIBRARY_NAME}_${TEST_LIBRARY_MAJOR_VER}_${TEST_LIBRARY_MINOR_VER}")

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

    set_target_properties(
        ${TEST_LIBRARY_NAME}
        PROPERTIES
            VERSION
                "${TEST_LIBRARY_MAJOR_VER}.${TEST_LIBRARY_MINOR_VER}"
            SOVERSION
                "${TEST_LIBRARY_MAJOR_VER}")
endif()

target_link_libraries(dynlib_ut PRIVATE
    gtest::gtest
    k4ainternal::dynlib
    k4ainternal::utcommon)

target_compile_definitions(dynlib_ut PRIVATE TEST_LIBRARY_MAJOR_VER=${TEST_LIBRARY_MAJOR_VER})
target_compile_definitions(dynlib_ut PRIVATE TEST_LIBRARY_MINOR_VER=${TEST_LIBRARY_MINOR_VER})
target_compile_definitions(dynlib_ut PRIVATE TEST_LIBRARY_NAME="${TEST_LIBRARY_NAME}")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # Search the same directory as the exectuable for the shared object
    target_link_libraries(dynlib_ut PRIVATE "-Wl,-rpath,'$$ORIGIN'")
endif()

k4a_add_tests(TARGET dynlib_ut TEST_TYPE UNIT)
