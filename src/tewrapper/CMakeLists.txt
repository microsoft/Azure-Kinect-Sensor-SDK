# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_tewrapper STATIC
            tewrapper.c
            )

# Consumers should #include <k4ainternal/tewrapper.h>
target_include_directories(k4a_tewrapper PUBLIC
    ${K4A_PRIV_INCLUDE_DIR})

target_link_libraries(k4a_tewrapper PUBLIC
    azure::aziotsharedutil
    k4ainternal::logging
    k4ainternal::deloader)

# Define alias for other targets to link against
add_library(k4ainternal::tewrapper ALIAS k4a_tewrapper)