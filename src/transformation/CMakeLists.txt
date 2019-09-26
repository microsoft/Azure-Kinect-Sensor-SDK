# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(k4a_transformation STATIC
            extrinsic_transformation.c
            intrinsic_transformation.c
            mode_specific_calibration.c
            rgbz.c
            transformation.c
            )

# Dependencies of this library
target_link_libraries(k4a_transformation PUBLIC 
    k4ainternal::math
    k4ainternal::deloader
    k4ainternal::tewrapper
    )

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "amd64.*|x86_64.*|AMD64.*|i686.*|i386.*|x86.*")
        target_compile_options(k4a_transformation PRIVATE "-msse4.1")
    endif()
endif()

# Define alias for other targets to link against
add_library(k4ainternal::transformation ALIAS k4a_transformation)
