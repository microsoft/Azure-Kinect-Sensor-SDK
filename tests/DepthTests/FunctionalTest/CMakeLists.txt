# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(depth_ft depth_ft.cpp)

target_link_libraries(depth_ft PRIVATE 
    k4a::k4a
    k4ainternal::utcommon
    gtest::gtest
    azure::aziotsharedutil)

k4a_add_tests(TARGET depth_ft HARDWARE_REQUIRED TEST_TYPE FUNCTIONAL)
