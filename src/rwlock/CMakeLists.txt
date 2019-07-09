# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_rwlock STATIC 
            rwlock_win32.c
            )

# Consumers should #include <k4ainternal/rwlock.h>
target_include_directories(k4a_rwlock PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Define alias for other targets to link against
add_library(k4ainternal::rwlock ALIAS k4a_rwlock)
