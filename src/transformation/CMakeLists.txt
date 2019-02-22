# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_transformation STATIC
            extrinsic_transformation.c
            intrinsic_transformation.c
            mode_specific_calibration.c
            rgbz.c
            transformation.c
            )

# Consumers should #include <k4ainternal/transformation.h>
target_include_directories(k4a_transformation PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_transformation PUBLIC 
    k4ainternal::math
    k4ainternal::deloader
    )

# Define alias for other targets to link against
add_library(k4ainternal::transformation ALIAS k4a_transformation)