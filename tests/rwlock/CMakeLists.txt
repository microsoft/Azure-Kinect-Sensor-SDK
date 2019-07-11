# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(rwlock_ft rwlock.cpp)

target_link_libraries(rwlock_ft PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4ainternal::rwlock
    k4ainternal::utcommon)

k4a_add_tests(TARGET rwlock_ft TEST_TYPE FUNCTIONAL)
