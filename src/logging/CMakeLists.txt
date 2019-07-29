# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_logging STATIC 
            logging.cpp
            )

# Consumers should #include <k4ainternal/logging.h>
target_include_directories(k4a_logging PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_logging PUBLIC 
    azure::aziotsharedutil
    spdlog::spdlog
    k4ainternal::rwlock
    k4ainternal::global
    )

# Define alias for other targets to link against
add_library(k4ainternal::logging ALIAS k4a_logging)