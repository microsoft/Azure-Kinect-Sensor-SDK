# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
find_package(OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} /usr/include/opencv2 ) # TODO is this good?

add_executable(green_screen main.cpp)
target_link_libraries(green_screen PRIVATE k4a::k4a ${OpenCV_LIBS})
