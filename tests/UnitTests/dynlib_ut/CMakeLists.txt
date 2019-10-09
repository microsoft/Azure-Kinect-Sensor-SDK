# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(dynlib_ut dynlib.cpp)


set(TEST_LIBRARY_NAME "test_dynlib")
set(TEST_LIBRARY_VERSION 1)

add_library(${TEST_LIBRARY_NAME} SHARED testdynlib.c)

include(GenerateExportHeader)
generate_export_header(${TEST_LIBRARY_NAME})

target_include_directories(${TEST_LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

    set_target_properties(
        ${TEST_LIBRARY_NAME}
        PROPERTIES
            OUTPUT_NAME
                "${TEST_LIBRARY_NAME}_${TEST_LIBRARY_VERSION}_0")

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

    set_target_properties(
        ${TEST_LIBRARY_NAME}
        PROPERTIES
            VERSION
                "${TEST_LIBRARY_VERSION}.0"
            SOVERSION
                "${TEST_LIBRARY_VERSION}")
endif()

target_link_libraries(dynlib_ut PRIVATE
    gtest::gtest
    k4ainternal::dynlib
    k4ainternal::utcommon)

target_compile_definitions(dynlib_ut PRIVATE TEST_LIBRARY_VERSION=${TEST_LIBRARY_VERSION})
target_compile_definitions(dynlib_ut PRIVATE TEST_LIBRARY_NAME="${TEST_LIBRARY_NAME}")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # Search the same directory as the exectuable for the shared object
    target_link_libraries(dynlib_ut PRIVATE "-Wl,-rpath,'$$ORIGIN'")
endif()

k4a_add_tests(TARGET dynlib_ut TEST_TYPE UNIT)
