# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_global STATIC 
            global.c
            )

# Consumers should #include <k4ainternal/global.h>
target_include_directories(k4a_global PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Define alias for other targets to link against
add_library(k4ainternal::global ALIAS k4a_global)
