# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(color_ft color_ft.cpp)

target_link_libraries(color_ft PRIVATE 
    k4a::k4a
    k4ainternal::utcommon
    gtest::gtest
    azure::aziotsharedutil)

k4a_add_tests(TARGET color_ft HARDWARE_REQUIRED TEST_TYPE FUNCTIONAL)
