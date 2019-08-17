# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

find_package(OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable(opencv_example main.cpp)

target_link_libraries(opencv_example PRIVATE k4a::k4a ${OpenCV_LIBS})
