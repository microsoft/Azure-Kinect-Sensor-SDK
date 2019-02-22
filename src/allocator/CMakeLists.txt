# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_allocator STATIC 
            allocator.c
            )

# Consumers should #include <k4ainternal/allocator.h>
target_include_directories(k4a_allocator PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_allocator PUBLIC 
    azure::aziotsharedutil
    k4ainternal::image
    k4ainternal::logging)

# Define alias for other targets to link against
add_library(k4ainternal::allocator ALIAS k4a_allocator)