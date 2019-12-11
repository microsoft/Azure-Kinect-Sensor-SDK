# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(k4a_cpp_ft k4a_cpp_ft.cpp)

target_link_libraries(k4a_cpp_ft PRIVATE
    k4ainternal::utcommon
    k4a::k4arecord
)

k4a_add_tests(TARGET k4a_cpp_ft TEST_TYPE FUNCTIONAL)
