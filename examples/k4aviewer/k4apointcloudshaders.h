/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4APOINTCLOUDSHADERS_H
#define K4APOINTCLOUDSHADERS_H

// clang-format off

constexpr char const PointCloudFragmentShader[] =
R"(
#version 330
varying vec3 fragment_color;

void main()
{
    gl_FragColor = vec4(fragment_color, 1);
}
)";

constexpr char const PointCloudVertexShader[] =
R"(
#version 330
attribute vec3 vertex_position;
attribute vec3 vertex_color;

varying vec3 fragment_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(vertex_position, 1);
    fragment_color = vertex_color;
}
)";

// clang-format on

#endif
