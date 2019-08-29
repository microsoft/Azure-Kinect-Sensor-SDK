# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

find_package(OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable(opencv_example main.cpp)

# OpenCV_LIBS, by default, is picking up the debug version of opencv on Windows even in release mode, which was causing a dependency on non-redistributable Visual Studio dlls.
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(opencv_example PRIVATE k4a::k4a ${OpenCV_DIR}/Opencv_world320.lib)
else()
    target_link_libraries(opencv_example PRIVATE k4a::k4a ${OpenCV_LIBS})
endif()
