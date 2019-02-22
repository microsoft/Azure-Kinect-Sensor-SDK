# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_math STATIC
            math.c
            )

# Consumers should #include <k4ainternal/math.h>
target_include_directories(k4a_math PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_math PUBLIC 
    )

# Define alias for other targets to link against
add_library(k4ainternal::math ALIAS k4a_math)