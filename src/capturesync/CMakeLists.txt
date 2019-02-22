# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_capturesync STATIC 
            capturesync.c
            )

# Consumers should #include <k4ainternal/capturesync.h>
target_include_directories(k4a_capturesync PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_capturesync PUBLIC 
    azure::aziotsharedutil
    k4ainternal::logging)

# Define alias for other targets to link against
add_library(k4ainternal::capturesync ALIAS k4a_capturesync)