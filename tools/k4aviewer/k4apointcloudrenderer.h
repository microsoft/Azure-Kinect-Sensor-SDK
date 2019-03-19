// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4APOINTCLOUDRENDERER_H
#define K4APOINTCLOUDRENDERER_H

// System headers
//

// Library headers
//
#include <k4a/k4a.hpp>
#include "k4aimgui_all.h"
#include "linmath.h"

// Project headers
//
#include "openglhelpers.h"

namespace k4aviewer
{
class PointCloudRenderer
{
public:
    PointCloudRenderer();
    ~PointCloudRenderer() = default;

    void UpdateViewProjection(linmath::mat4x4 view, linmath::mat4x4 projection);

    GLenum UpdatePointClouds(const k4a::image &color, const OpenGL::Texture &pointCloudTexture);

    GLenum Render();

    void SetPointSize(int pointSize);
    int GetPointSize() const;
    void EnableShading(bool enableShading);
    bool ShadingIsEnabled() const;

    PointCloudRenderer(PointCloudRenderer &) = delete;
    PointCloudRenderer(PointCloudRenderer &&) = delete;
    PointCloudRenderer &operator=(PointCloudRenderer &) = delete;
    PointCloudRenderer &operator=(PointCloudRenderer &&) = delete;

private:
    linmath::mat4x4 m_view;
    linmath::mat4x4 m_projection;

    // Render settings
    int m_pointSize = 2;
    bool m_enableShading = true;

    // Point Array Size
    GLsizei m_vertexArraySizeBytes = 0;

    // OpenGL resources
    OpenGL::Program m_shaderProgram;
    GLint m_viewIndex;
    GLint m_projectionIndex;
    GLint m_enableShadingIndex;
    GLint m_pointCloudTextureIndex;

    OpenGL::VertexArray m_vertexArrayObject = OpenGL::VertexArray(true);
    OpenGL::Buffer m_vertexColorBufferObject = OpenGL::Buffer(true);
};
} // namespace k4aviewer
#endif
