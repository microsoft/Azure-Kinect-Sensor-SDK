# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(K4A_COLOR_SYSTEM_SOURCES mfcamerareader.cpp)
    set(K4A_COLOR_SYSTEM_DEPENDENCIES 
        mf.lib
        mfplat.lib
        mfreadwrite.lib
        mfuuid.lib
        cfgmgr32.lib)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(K4A_COLOR_SYSTEM_SOURCES uvc_camerareader.cpp)
    set(K4A_COLOR_SYSTEM_DEPENDENCIES libuvc::libuvc libjpeg-turbo::libjpeg-turbo)
endif()

add_library(k4a_color STATIC
            color.cpp
            ${K4A_COLOR_SYSTEM_SOURCES})

# Consumers should #include <k4ainternal/color.h>
target_include_directories(k4a_color PUBLIC 
    ${K4A_PRIV_INCLUDE_DIR})

# Dependencies of this library
target_link_libraries(k4a_color PUBLIC
                      k4ainternal::logging
                      ${K4A_COLOR_SYSTEM_DEPENDENCIES})

# Define alias for other targets to link against
add_library(k4ainternal::color ALIAS k4a_color)
