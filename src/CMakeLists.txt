# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Consumers of this include directory need to #include "k4ainternal/header.h"
set(K4A_PRIV_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../include ${CMAKE_CURRENT_BINARY_DIR}/sdk/include)

# Add folders in Alphabetical order to help reduce merge issues
add_subdirectory(allocator)
add_subdirectory(calibration)
add_subdirectory(capturesync)
add_subdirectory(color)
add_subdirectory(color_mcu)
add_subdirectory(depth)
add_subdirectory(depth_mcu)
add_subdirectory(deloader)
add_subdirectory(dewrapper)
add_subdirectory(dynlib)
add_subdirectory(firmware)
add_subdirectory(global)
add_subdirectory(image)
add_subdirectory(imu)
add_subdirectory(logging)
add_subdirectory(math)
add_subdirectory(queue)
add_subdirectory(record)
add_subdirectory(rwlock)
add_subdirectory(sdk)
add_subdirectory(tewrapper)
add_subdirectory(transformation)
add_subdirectory(usbcommand)
