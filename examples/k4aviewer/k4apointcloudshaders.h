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
#version 430
varying vec4 fragmentColor;

void main()
{
    gl_FragColor = fragmentColor;
}
)";

constexpr char const PointCloudVertexShader[] =
R"(
#version 430
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec3 vertexNeighbor;

varying vec4 fragmentColor;

uniform mat4 view;
uniform mat4 projection;
uniform bool enableShading;

void main()
{
    gl_Position = projection * view * vec4(vertexPosition, 1);

    if (enableShading)
    {
        vec3 lightPosition = vec3(0, 0, 0);
        float occlusion = length(vertexPosition - vertexNeighbor) * 20.0f;
        float diffuse = 1.0f - clamp(occlusion, 0.0f, 0.6f);

        float distance = length(lightPosition - vertexPosition);
        // Attenuation term for light source that covers distance up to 50 meters
        // http://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        fragmentColor = vec4(attenuation * diffuse * vertexColor.rgb, vertexColor.a);
    }
    else
    {
        fragmentColor = vertexColor;
    }
}
)";

// clang-format on

#endif
