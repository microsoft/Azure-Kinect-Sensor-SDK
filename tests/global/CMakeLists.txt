# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(global_ft global.cpp)

target_link_libraries(global_ft PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4ainternal::global
    k4ainternal::utcommon)

k4a_add_tests(TARGET global_ft TEST_TYPE FUNCTIONAL)
