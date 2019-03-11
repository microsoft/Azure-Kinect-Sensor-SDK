# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(cpp_projection_ft cpp_projection_ft.cpp)

target_link_libraries(cpp_projection_ft PRIVATE
    k4a::k4a
    k4ainternal::utcommon
    gtest::gtest)

k4a_add_tests(TARGET cpp_projection_ft HARDWARE_REQUIRED TEST_TYPE FUNCTIONAL)
