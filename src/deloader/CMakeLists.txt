# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_deloader STATIC
            deloader.cpp
            )

# Consumers should #include <k4ainternal/dewrapper.h>
target_include_directories(k4a_deloader PUBLIC
    ${K4A_PRIV_INCLUDE_DIR})

target_link_libraries(k4a_deloader PUBLIC
    k4ainternal::allocator
    k4ainternal::dynlib
    k4ainternal::logging)

# Define alias for other targets to link against
add_library(k4ainternal::deloader ALIAS k4a_deloader)