# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(azure_c_shared_test
    main.cpp
    threading.cpp)

target_link_libraries(azure_c_shared_test PRIVATE
    azure::aziotsharedutil
    gtest::gtest)

k4a_add_tests(TARGET azure_c_shared_test TEST_TYPE UNIT)
