# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(rwlock_ut rwlock.cpp)

target_link_libraries(rwlock_ut PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4ainternal::rwlock
    k4ainternal::utcommon)

k4a_add_tests(TARGET rwlock_ut TEST_TYPE UNIT)
