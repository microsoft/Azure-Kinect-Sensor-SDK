# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(k4aTest)

# Only run green screen tests if OpenCV is installed
find_package(OpenCV)
add_executable(executables_ft executables_ft.cpp) # Single Device Tests
add_executable(executables_ft_custom executables_ft.cpp) # Multi Device Tests
if (OpenCV_FOUND)
    target_compile_definitions(executables_ft PRIVATE -DUSE_OPENCV=1 )
    target_compile_definitions(executables_ft_custom PRIVATE -DUSE_OPENCV=1 )
endif()
target_compile_definitions(executables_ft PRIVATE -DUSE_CUSTOM_TEST_CONFIGURATION=0 )
target_compile_definitions(executables_ft_custom PRIVATE -DUSE_CUSTOM_TEST_CONFIGURATION=1 )

target_link_libraries(executables_ft PRIVATE 
    k4a::k4a
    k4ainternal::utcommon
    gtest::gtest)

target_link_libraries(executables_ft_custom PRIVATE 
    k4a::k4a
    k4ainternal::utcommon
    gtest::gtest)

k4a_add_tests(TARGET executables_ft HARDWARE_REQUIRED TEST_TYPE FUNCTIONAL)
k4a_add_tests(TARGET executables_ft_custom HARDWARE_REQUIRED TEST_TYPE FUNCTIONAL_CUSTOM)
