# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

find_package(OpenCV REQUIRED)
include_directories(
    .
	../kahelpers/inc/
    ${OPENCV_INCLUDE_DIRS}
)

add_executable(mkv2images
    mkv2images.cpp
	../kahelpers/inc/kahelpers.h)

target_link_libraries(mkv2images PRIVATE
    k4a::k4a
    ${OPENCV_LIBRARIES})