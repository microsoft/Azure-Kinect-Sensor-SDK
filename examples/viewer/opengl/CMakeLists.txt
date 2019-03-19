# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

find_package(OpenGL REQUIRED)
include_directories(
    .
    ${OPENGL_INCLUDE_DIRS}
)

add_executable(viewer_opengl
    main.cpp
    texture.cpp
    viewerwindow.cpp)

target_link_libraries(viewer_opengl PRIVATE
    k4a::k4a
    glfw::glfw
    imgui::imgui
    ${OPENGL_LIBRARIES})