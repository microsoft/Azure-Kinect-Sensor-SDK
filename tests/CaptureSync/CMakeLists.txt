# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(capturesync_ut capturesync.cpp)

target_link_libraries(capturesync_ut PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4ainternal::allocator
    k4ainternal::capturesync
    k4ainternal::image
    k4ainternal::queue
    k4ainternal::utcommon)

k4a_add_tests(TARGET capturesync_ut TEST_TYPE UNIT)
