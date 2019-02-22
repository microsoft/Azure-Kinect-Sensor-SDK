# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_dewrapper STATIC
            dewrapper.c
            )

# Consumers should #include <k4ainternal/dewrapper.h>
target_include_directories(k4a_dewrapper PUBLIC
    ${K4A_PRIV_INCLUDE_DIR})

target_link_libraries(k4a_dewrapper PUBLIC
    azure::aziotsharedutil
    k4ainternal::allocator
    k4ainternal::calibration
    k4ainternal::logging
    k4ainternal::queue
    k4ainternal::deloader)

# Define alias for other targets to link against
add_library(k4ainternal::dewrapper ALIAS k4a_dewrapper)