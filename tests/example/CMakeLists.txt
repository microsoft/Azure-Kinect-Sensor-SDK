# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(k4a_example_test test.cpp)
target_link_libraries(k4a_example_test PRIVATE k4a::k4a)
target_link_libraries(k4a_example_test PRIVATE gtest::gtest)

k4a_add_tests(TARGET k4a_example_test HARDWARE_REQUIRED TEST_TYPE FUNCTIONAL)
