# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Add a test
#
# k4a_add_tests(TARGET <target>
#               TEST_TYPE <UNIT | FUNCTIONAL | STRESS | PERF>
#               WORKING_DIRECTORY <working_directory>
#               [HARDWARE_REQUIRED])
#
# TARGET - Name of build target that is the test binary
#
# TEST_TYPE - The type of test this build target is running.
#             UNIT - Tests meant to run on build machine. These tests run very
#                    quickly (<1s), and reproducible, and do not require
#                    hardware.
#
#             FUNCTIONAL - Tests meant to run on test machine. These tests run
#                          quickly (<10s), may require hardware, run on PCs that
#                          meet min spec requirements, and are reproducible. 
#                          These tests must also be capable of working on a 
#                          single Azure Kinect and not require any additional 
#                          setup to succeed.
#
#             FUNCTIONAL_CUSTOM - Similar to FUNCTIONAL tests above. These tests
#                          however are allowed to have additional physical 
#                          requirements like lighting, multiple devices, or
#                          visible chessboard pattern for calibration.
#
#             STRESS - Tests that run repeatedly and look for statistical
#                      failures
#
#             PERF - Tests that run on target environment and report perf stats.
#                    These tests are not pass fail
#
# WORKING_DIRECTORY - Working directory to run the UnitTest
#
# Adds a googletest framework test to the K4A test list
#
# Results of the test will be output in XML form and consumed by the CI system
#

include(GoogleTest)

get_property(is_defined GLOBAL PROPERTY TEST_TYPES DEFINED)
if (NOT is_defined)
    define_property(GLOBAL PROPERTY TEST_TYPES
    BRIEF_DOCS "List of types of tests"
    FULL_DOCS "Contains full list of all test types")

    set(TEST_TYPES "UNIT" "FUNCTIONAL" "STRESS" "PERF" "FIRMWARE" "FUNCTIONAL_CUSTOM")
    set_property(GLOBAL PROPERTY TEST_TYPES ${TEST_TYPES})

    foreach(TEST_TYPE ${TEST_TYPES})
        if ("${CMAKE_CONFIGURATION_TYPES}" STREQUAL "")
            get_property(is_defined GLOBAL PROPERTY ${TEST_TYPE}_TEST_LIST DEFINED)
            if(is_defined)
                message(FATAL_ERROR "${${TEST_TYPE}_TEST_LIST} is already defined.")
            endif()
            define_property(GLOBAL PROPERTY ${TEST_TYPE}_TEST_LIST
            BRIEF_DOCS "List of ${TEST_TYPE} tests"
            FULL_DOCS "Contains full list of tests of type ${TEST_TYPE}")
        else()
            foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
                get_property(is_defined GLOBAL PROPERTY ${CONFIG}_${TEST_TYPE}_TEST_LIST DEFINED)
                if(is_defined)
                    message(FATAL_ERROR "${${CONFIG}_${TEST_TYPE}_TEST_LIST} is already defined.")
                endif()
                define_property(GLOBAL PROPERTY ${CONFIG}_${TEST_TYPE}_TEST_LIST
                BRIEF_DOCS "List of ${CONFIG} ${TEST_TYPE} tests"
                FULL_DOCS "Contains full list of tests of type ${TEST_TYPE} for ${CONFIG} builds")
            endforeach()
        endif()
    endforeach()

    unset(TEST_TYPES)
endif()

# string(JOIN ...) was added in CMake 3.11 and thus can not be used.
# string_join was written to mimic string(JOIN ...) functionality and
# interface.
function(string_join GLUE OUTPUT VALUES)
    set(_TEMP_VALUES ${VALUES} ${ARGN})
    string(REPLACE ";" "${GLUE}" _TEMP_STR "${_TEMP_VALUES}")
    set(${OUTPUT} "${_TEMP_STR}" PARENT_SCOPE)
endfunction()

function(k4a_add_tests)
    cmake_parse_arguments(
        K4A_ADD_TESTS                        # Prefix
        "HARDWARE_REQUIRED"                  # Options
        "TARGET;TEST_TYPE;WORKING_DIRECTORY" # One value keywords
        ""                                   # Multi value keywords
        ${ARGN})                             # args...

    if (NOT K4A_ADD_TESTS_TARGET)
        message(FATAL_ERROR "No TARGET supplied to k4a_add_tests")
    endif()

    if (NOT K4A_ADD_TESTS_TEST_TYPE)
        message(FATAL_ERROR "No TEST_TYPE supplied to k4a_add_tests")
    endif()

    if (NOT K4A_ADD_TESTS_WORKING_DIRECTORY)
        set(K4A_ADD_TESTS_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
    endif()

    get_property(TEST_TYPES GLOBAL PROPERTY TEST_TYPES)

    if (NOT ${K4A_ADD_TESTS_TEST_TYPE} IN_LIST TEST_TYPES)
        string_join(" " TEST_TYPES_STR ${TEST_TYPES})
        message(
            FATAL_ERROR
            "Unknown TEST_TYPE ${K4A_ADD_TESTS_TEST_TYPE} for target \
            ${K4A_ADD_TESTS_TARGET}. Valid options are: ${TEST_TYPES_STR}")
    endif()

    string(TOLOWER ${K4A_ADD_TESTS_TEST_TYPE} TEST_TYPE)
    set(HARDWARE_REQUIRED ${K4A_ADD_TESTS_HARDWARE_REQUIRED})

    if ((${TEST_TYPE} STREQUAL "unit") AND (${HARDWARE_REQUIRED}))
        message(FATAL_ERROR "K4A test (${K4A_ADD_TESTS_TARGET}) may not
        be both the UNIT test type and require hardware")
    endif()

    set(LABELS "")

    if (${HARDWARE_REQUIRED})
        list(APPEND LABELS "hardware")
    endif()

    list(APPEND LABELS ${TEST_TYPE})

    # This is a work around to issue 17812 with cmake. In order to set
    # multiple labels on a test, we need to pass in "LABELS" in front of each
    # label. For example, if we have the labels "hardware" and "functional" we
    # should pass in "LABELS;hardware;LABELS;functional" to cmake.
    # See issue here: https://gitlab.kitware.com/cmake/cmake/issues/17812
    set(GTEST_PROPERTIES)
    foreach(LABEL ${LABELS})
        list(APPEND GTEST_PROPERTIES "LABELS;${LABEL}")
    endforeach()

    # Turn on logging to stdout
    if (NOT DEFINED ENV{K4A_LOG_LEVEL})
        list(APPEND GTEST_PROPERTIES "ENVIRONMENT;K4A_LOG_LEVEL=I;")
    endif()

    if (NOT DEFINED ENV{K4A_ENABLE_LOG_TO_STDOUT})
        list(APPEND GTEST_PROPERTIES "ENVIRONMENT;K4A_ENABLE_LOG_TO_STDOUT=1")
    endif()


    set(TIMEOUT_VAR_NAME "DISCOVERY_TIMEOUT")
    if ((${CMAKE_VERSION} VERSION_EQUAL "3.10.1") OR (${CMAKE_VERSION} VERSION_EQUAL "3.10.2"))
        set(TIMEOUT_VAR_NAME "TIMEOUT")
    endif()

    gtest_discover_tests(
        ${K4A_ADD_TESTS_TARGET}
        EXTRA_ARGS
            "--gtest_output=xml:TEST-${K4A_ADD_TESTS_TARGET}.xml"
        WORKING_DIRECTORY
            ${K4A_ADD_TESTS_WORKING_DIRECTORY}
        ${TIMEOUT_VAR_NAME}
            60
        PROPERTIES
            "${GTEST_PROPERTIES}")

    if ("${CMAKE_CONFIGURATION_TYPES}" STREQUAL "")
        set_property(
            GLOBAL
            APPEND
            PROPERTY ${TEST_TYPE}_TEST_LIST
            ${K4A_ADD_TESTS_TARGET}${CMAKE_EXECUTABLE_SUFFIX})

        get_property(TESTS GLOBAL PROPERTY ${TEST_TYPE}_TEST_LIST)

        string_join("\n" TESTS_STR ${TESTS})
        file(
            WRITE
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TEST_TYPE}_test_list.txt
            ${TESTS_STR})
    else()
        foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
            set_property(
                GLOBAL
                APPEND
                PROPERTY ${CONFIG}_${TEST_TYPE}_TEST_LIST
                "${K4A_ADD_TESTS_TARGET}${CMAKE_EXECUTABLE_SUFFIX}")

            get_property(TESTS GLOBAL PROPERTY ${CONFIG}_${TEST_TYPE}_TEST_LIST)

            string_join("\n" TESTS_STR ${TESTS})
            file(
                WRITE
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG}/${TEST_TYPE}_test_list.txt
                ${TESTS_STR})
        endforeach()
    endif()
endfunction()
