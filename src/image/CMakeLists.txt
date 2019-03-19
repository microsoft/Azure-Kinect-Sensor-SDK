# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_image STATIC 
            image.c
            )

# Consumers should #include <k4ainternal/image.h>
target_include_directories(k4a_image PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_image PUBLIC 
    azure::aziotsharedutil
    k4ainternal::allocator
    k4ainternal::logging)

# Define alias for other targets to link against
add_library(k4ainternal::image ALIAS k4a_image)