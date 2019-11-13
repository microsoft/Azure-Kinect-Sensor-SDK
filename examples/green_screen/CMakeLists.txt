# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
add_executable(green_screen main.cpp)
target_include_directories( green_screen PRIVATE ${OpenCV_INCLUDE_DIRS} )
target_link_libraries(green_screen PRIVATE k4a::k4a ${OpenCV_LIBS})
