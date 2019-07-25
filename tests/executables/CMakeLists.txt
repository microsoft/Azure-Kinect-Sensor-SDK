# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(k4aTest)

add_executable(executables_ft executables_ft.cpp)
target_link_libraries(executables_ft PRIVATE 
    k4a::k4a
    k4ainternal::utcommon
    gtest::gtest)

k4a_add_tests(TARGET executables_ft HARDWARE_REQUIRED TEST_TYPE FUNCTIONAL)
