# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
add_library(k4a_rwlock STATIC 
            rwlock_linux.c
            )
else()
add_library(k4a_rwlock STATIC 
            rwlock_win32.c
            )
endif()

# Consumers should #include <k4ainternal/rwlock.h>
target_include_directories(k4a_rwlock PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Define alias for other targets to link against
add_library(k4ainternal::rwlock ALIAS k4a_rwlock)
