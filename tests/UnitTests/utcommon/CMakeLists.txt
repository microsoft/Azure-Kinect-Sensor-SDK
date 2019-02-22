# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(utcommon STATIC utcommon.cpp)

target_link_libraries(utcommon PUBLIC 
    gtest::gtest
    gtest::gmock
    k4ainternal::logging
)

target_include_directories(utcommon PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc)

# Define alias for other targets to link against
add_library(k4ainternal::utcommon ALIAS utcommon)