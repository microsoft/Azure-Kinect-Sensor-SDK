// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4APOINTCLOUDSHADERS_H
#define K4APOINTCLOUDSHADERS_H

// clang-format off

constexpr char const PointCloudFragmentShader[] =
R"(
#version 430

in vec4 vertexColor;
out vec4 fragmentColor;

uniform bool enableShading;

void main()
{
    if (vertexColor.a == 0.0f)
    {
        discard;
    }

    fragmentColor = vertexColor;
}
)";

constexpr char const PointCloudVertexShader[] =
R"(
#version 430
layout(location = 0) in vec4 inColor;

out vec4 vertexColor;

uniform mat4 view;
uniform mat4 projection;
layout(rgba32f) readonly uniform image2D pointCloudTexture;
uniform bool enableShading;

bool GetPoint3d(in vec2 pointCloudSize, in ivec2 point2d, out vec3 point3d)
{
    if (point2d.x < 0 || point2d.x >= pointCloudSize.x ||
        point2d.y < 0 || point3d.y >= pointCloudSize.y)
    {
        return false;
    }

    point3d = imageLoad(pointCloudTexture, point2d).xyz;
    if (point3d.z <= 0)
    {
        return false;
    }

    return true;
}

void main()
{
    ivec2 pointCloudSize = imageSize(pointCloudTexture);
    ivec2 currentDepthPixelCoordinates = ivec2(gl_VertexID % pointCloudSize.x, gl_VertexID / pointCloudSize.x);
    vec3 vertexPosition = imageLoad(pointCloudTexture, currentDepthPixelCoordinates).xyz;

    gl_Position = projection * view * vec4(vertexPosition, 1);

    vertexColor = inColor;

    // Pass along the 'invalid pixel' flag as the alpha channel
    //
    if (vertexPosition.z == 0.0f)
    {
        vertexColor.a = 0.0f;
    }

    if (enableShading)
    {
        // Compute the location of the closest neighbor pixel to compute lighting
        //
        vec3 closestNeighbor = vertexPosition;

        // If no neighbors have data, default to 1 meter behind point.
        //
        closestNeighbor.z += 1.0f;

        vec3 outPoint;
        if (GetPoint3d(pointCloudSize, currentDepthPixelCoordinates - ivec2(1, 0), outPoint))
        {
            if (closestNeighbor.z > outPoint.z)
            {
                closestNeighbor = outPoint;
            }
        }
        if (GetPoint3d(pointCloudSize, currentDepthPixelCoordinates + ivec2(1, 0), outPoint))
        {
            if (closestNeighbor.z > outPoint.z)
            {
                closestNeighbor = outPoint;
            }
        }
        if (GetPoint3d(pointCloudSize, currentDepthPixelCoordinates - ivec2(0, 1), outPoint))
        {
            if (closestNeighbor.z > outPoint.z)
            {
                closestNeighbor = outPoint;
            }
        }
        if (GetPoint3d(pointCloudSize, currentDepthPixelCoordinates + ivec2(0, 1), outPoint))
        {
            if (closestNeighbor.z > outPoint.z)
            {
                closestNeighbor = outPoint;
            }
        }

        vec3 lightPosition = vec3(0, 0, 0);
        float occlusion = length(vertexPosition - closestNeighbor) * 20.0f;
        float diffuse = 1.0f - clamp(occlusion, 0.0f, 0.6f);

        float distance = length(lightPosition - vertexPosition);

        // Attenuation term for light source that covers distance up to 50 meters
        // http://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation
        //
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        vertexColor = vec4(attenuation * diffuse * vertexColor.rgb, vertexColor.a);
    }
}
)";

// clang-format on

#endif
