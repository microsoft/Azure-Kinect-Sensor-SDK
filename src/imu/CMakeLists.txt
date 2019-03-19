# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_imu STATIC
            imu.c
            )

# Consumers should #include <k4ainternal/imu.h>
target_include_directories(k4a_imu PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_imu PUBLIC 
    k4ainternal::logging
    k4ainternal::usb_cmd
    )

# Define alias for other targets to link against
add_library(k4ainternal::imu ALIAS k4a_imu)