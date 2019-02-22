# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(allocator_ut allocator.cpp)

target_link_libraries(allocator_ut PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4ainternal::allocator
    k4ainternal::image
    k4ainternal::utcommon)

k4a_add_tests(TARGET allocator_ut TEST_TYPE UNIT)
