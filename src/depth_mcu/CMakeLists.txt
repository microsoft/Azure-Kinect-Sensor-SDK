# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_depth_mcu STATIC 
            depth_mcu.c
            )

# Consumers should #include <k4ainternal/depth_mcu.h>
target_include_directories(k4a_depth_mcu PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_depth_mcu PUBLIC 
    k4ainternal::image
    k4ainternal::logging
    k4ainternal::usb_cmd)

# Define alias for other targets to link against
add_library(k4ainternal::depth_mcu ALIAS k4a_depth_mcu)