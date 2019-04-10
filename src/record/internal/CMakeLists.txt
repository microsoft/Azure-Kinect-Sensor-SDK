# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Define internal library for testing usage
add_library(k4a_record STATIC 
    iocallback.cpp
    matroska_write.cpp
)
add_library(k4a_playback STATIC 
    iocallback.cpp
    matroska_read.cpp
)

# Consumers should #include <k4ainternal/record_write.h>
target_include_directories(k4a_record PUBLIC
    ${K4A_PRIV_INCLUDE_DIR})

# Consumers should #include <k4ainternal/record_read.h>
target_include_directories(k4a_playback PUBLIC
    ${K4A_PRIV_INCLUDE_DIR})

target_link_libraries(k4a_record PUBLIC 
    k4a::k4a
    k4ainternal::logging
    ebml::ebml
    matroska::matroska
)

target_link_libraries(k4a_playback PUBLIC 
    k4a::k4a
    k4ainternal::logging
    ebml::ebml
    matroska::matroska
    libyuv::libyuv
    libjpeg-turbo::libjpeg-turbo
)

# Define alias for other targets to link against
add_library(k4ainternal::record ALIAS k4a_record)
add_library(k4ainternal::playback ALIAS k4a_playback)
