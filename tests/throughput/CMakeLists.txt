# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(throughput_perf throughput_perf.cpp)

target_compile_definitions(throughput_perf PRIVATE _CRT_SECURE_NO_WARNINGS)

target_link_libraries(throughput_perf PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4a::k4a
    k4ainternal::logging
    k4ainternal::utcommon)

k4a_add_tests(TARGET throughput_perf HARDWARE_REQUIRED TEST_TYPE PERF)
