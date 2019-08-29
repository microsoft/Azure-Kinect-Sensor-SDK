# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
find_package(OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable(green_screen main.cpp)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(green_screen PRIVATE k4a::k4a ${OpenCV_DIR}/Opencv_world320.lib)
else()
    target_link_libraries(green_screen PRIVATE k4a::k4a ${OpenCV_LIBS})
endif()
