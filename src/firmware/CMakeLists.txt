# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_firmware STATIC 
            firmware.c)

# Consumers should #include <k4ainternal/firmware.h>
target_include_directories(k4a_firmware PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_firmware PUBLIC 
    k4ainternal::color_mcu
    k4ainternal::logging)

# Define alias for other targets to link against
add_library(k4ainternal::firmware ALIAS k4a_firmware)
