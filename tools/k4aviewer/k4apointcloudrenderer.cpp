// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4apointcloudrenderer.h"

// System headers
//
#include <algorithm>
#include <sstream>

// Library headers
//

// Project headers
//
#include "gpudepthtopointcloudconverter.h"
#include "k4apixel.h"
#include "k4apointcloudshaders.h"
#include "k4apointcloudviewcontrol.h"
#include "k4aviewerutil.h"
#include "perfcounter.h"

using namespace linmath;
using namespace k4aviewer;

PointCloudRenderer::PointCloudRenderer()
{
    mat4x4_identity(m_view);
    mat4x4_identity(m_projection);

    // Context Settings
    glEnable(GL_PROGRAM_POINT_SIZE);

    OpenGL::Shader vertexShader(GL_VERTEX_SHADER, PointCloudVertexShader);
    OpenGL::Shader fragmentShader(GL_FRAGMENT_SHADER, PointCloudFragmentShader);

    m_shaderProgram.AttachShader(std::move(vertexShader));
    m_shaderProgram.AttachShader(std::move(fragmentShader));
    m_shaderProgram.Link();

    m_viewIndex = glGetUniformLocation(m_shaderProgram.Id(), "view");
    m_projectionIndex = glGetUniformLocation(m_shaderProgram.Id(), "projection");
    m_enableShadingIndex = glGetUniformLocation(m_shaderProgram.Id(), "enableShading");
    m_pointCloudTextureIndex = glGetUniformLocation(m_shaderProgram.Id(), "pointCloudTexture");
}

void PointCloudRenderer::UpdateViewProjection(mat4x4 view, mat4x4 projection)
{
    mat4x4_dup(m_view, view);
    mat4x4_dup(m_projection, projection);
}

GLenum PointCloudRenderer::UpdatePointClouds(const k4a::image &color, const OpenGL::Texture &pointCloudTexture)
{
    glBindVertexArray(m_vertexArrayObject.Id());

    // Vertex Colors
    //
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexColorBufferObject.Id());

    const int colorImageSizeBytes = static_cast<int>(color.get_size());

    if (m_vertexArraySizeBytes != colorImageSizeBytes)
    {
        m_vertexArraySizeBytes = colorImageSizeBytes;
        glBufferData(GL_ARRAY_BUFFER, m_vertexArraySizeBytes, nullptr, GL_STREAM_DRAW);
    }

    GLubyte *vertexMappedBuffer = reinterpret_cast<GLubyte *>(
        glMapBufferRange(GL_ARRAY_BUFFER, 0, colorImageSizeBytes, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    if (!vertexMappedBuffer)
    {
        return glGetError();
    }

    const GLubyte *colorSrc = reinterpret_cast<const GLubyte *>(color.get_buffer());
    std::copy(colorSrc, colorSrc + colorImageSizeBytes, vertexMappedBuffer);
    if (!glUnmapBuffer(GL_ARRAY_BUFFER))
    {
        return glGetError();
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

    glUseProgram(m_shaderProgram.Id());

    // Uniforms
    //
    // Bind our point cloud texture
    //
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pointCloudTexture.Id());
    glBindImageTexture(0,
                       pointCloudTexture.Id(),
                       0,
                       GL_FALSE,
                       0,
                       GL_READ_ONLY,
                       GpuDepthToPointCloudConverter::PointCloudTextureFormat);
    glUniform1i(m_pointCloudTextureIndex, 0);

    glBindVertexArray(0);

    return glGetError();
}

GLenum PointCloudRenderer::Render()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(static_cast<GLfloat>(m_pointSize));

    glUseProgram(m_shaderProgram.Id());

    // Update view/projective matrices in shader
    //
    glUniformMatrix4fv(m_viewIndex, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(m_view));
    glUniformMatrix4fv(m_projectionIndex, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(m_projection));

    // Update render settings in shader
    //
    glUniform1i(m_enableShadingIndex, static_cast<GLint>(m_enableShading));

    // Render point cloud
    //
    glBindVertexArray(m_vertexArrayObject.Id());
    glDrawArrays(GL_POINTS, 0, m_vertexArraySizeBytes / static_cast<GLsizei>(sizeof(BgraPixel)));

    glBindVertexArray(0);

    return glGetError();
}

int PointCloudRenderer::GetPointSize() const
{
    return m_pointSize;
}

void PointCloudRenderer::SetPointSize(const int pointSize)
{
    m_pointSize = pointSize;
}

void PointCloudRenderer::EnableShading(bool enableShading)
{
    m_enableShading = enableShading;
}

bool PointCloudRenderer::ShadingIsEnabled() const
{
    return m_enableShading;
}
