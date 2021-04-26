# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_modes STATIC
            modes.c
            )

# Consumers should #include <k4ainternal/modes.h>
target_include_directories(k4a_modes PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})


if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "amd64.*|x86_64.*|AMD64.*|i686.*|i386.*|x86.*")
        target_compile_options(k4a_modes PRIVATE "-msse4.1")
    endif()
endif()

# Define alias for other targets to link against
add_library(k4ainternal::modes ALIAS k4a_modes)
