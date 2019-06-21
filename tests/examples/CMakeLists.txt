# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(k4aTest)

add_executable(examples_ft examples_ft.cpp)
target_link_libraries(examples_ft PRIVATE 
    k4a::k4a
    k4ainternal::utcommon
    gtest::gtest)

k4a_add_tests(TARGET examples_ft HARDWARE_REQUIRED TEST_TYPE FUNCTIONAL)
