# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
add_executable(green_screen main.cpp)
target_include_directories( green_screen PRIVATE ${OpenCV_INCLUDE_DIRS} )


# OpenCV_LIBS, by default, is picking up the debug version of opencv on Windows even in release mode, which was causing a dependency on non-redistributable Visual Studio dlls.
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_link_libraries(green_screen PRIVATE k4a::k4a ${OpenCV_DIR}/Opencv_world320d.lib)
    else()
        target_link_libraries(green_screen PRIVATE k4a::k4a ${OpenCV_DIR}/Opencv_world320.lib)
    endif()        
else()
    target_link_libraries(green_screen PRIVATE k4a::k4a ${OpenCV_LIBS})
endif()
