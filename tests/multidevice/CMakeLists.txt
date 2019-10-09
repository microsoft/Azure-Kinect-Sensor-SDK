# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(multidevice_ft multidevice.cpp)

target_link_libraries(multidevice_ft PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4a::k4a
    k4ainternal::utcommon)

k4a_add_tests(TARGET multidevice_ft TEST_TYPE FUNCTIONAL_CUSTOM)
