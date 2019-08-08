# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(calibration)
add_subdirectory(enumerate)
add_subdirectory(playback_external_sync)
add_subdirectory(k4arecord_custom_track)
add_subdirectory(fastpointcloud)
add_subdirectory(streaming)
add_subdirectory(transformation)
add_subdirectory(undistort)
add_subdirectory(viewer)

find_package(OpenCV)
if (OpenCV_FOUND)
    add_subdirectory(green_screen)
    add_subdirectory(opencv_compatibility)
endif ()
