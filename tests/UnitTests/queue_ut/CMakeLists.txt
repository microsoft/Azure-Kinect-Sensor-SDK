# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(queue_ut queue.cpp)

target_link_libraries(queue_ut PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4ainternal::allocator
    k4ainternal::image
    k4ainternal::queue
    k4ainternal::utcommon)

k4a_add_tests(TARGET queue_ut TEST_TYPE UNIT)
