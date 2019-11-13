# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(opencv_example main.cpp)
target_include_directories( opencv_example PRIVATE ${OpenCV_INCLUDE_DIRS} )
target_link_libraries(opencv_example PRIVATE k4a::k4a ${OpenCV_LIBS})
