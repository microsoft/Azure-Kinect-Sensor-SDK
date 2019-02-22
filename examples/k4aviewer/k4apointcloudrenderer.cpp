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
#include "assertionexception.h"
#include "k4apointcloudshaders.h"
#include "k4apointcloudviewcontrol.h"

using namespace linmath;
using namespace k4aviewer;

namespace
{
// Shader validation functions
// Should only fail if there's some sort of syntax error in the shader that makes
// it unable to compile/link, so we crash if these fail.
//
void ValidateShader(GLuint shaderIndex)
{
    GLint success = GL_FALSE;
    char infoLog[512];
    glGetShaderiv(shaderIndex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shaderIndex, 512, nullptr, infoLog);
        std::stringstream errorBuilder;
        errorBuilder << "Shader compilation error: " << std::endl << infoLog;
        throw AssertionException(errorBuilder.str().c_str());
    }
}

void ValidateProgram(GLuint programIndex)
{
    GLint success = GL_FALSE;
    char infoLog[512];
    glGetProgramiv(programIndex, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programIndex, 512, nullptr, infoLog);
        std::stringstream errorBuilder;
        errorBuilder << "Program compilation error: " << std::endl << infoLog;
        throw AssertionException(errorBuilder.str().c_str());
    }
}
} // namespace

PointCloudRenderer::PointCloudRenderer()
{
    mat4x4_identity(m_view);
    mat4x4_identity(m_projection);

    // Context Settings
    glEnable(GL_PROGRAM_POINT_SIZE);

    m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *vertexShaderCodeBuffer = PointCloudVertexShader;
    glShaderSource(m_vertexShader, 1, &vertexShaderCodeBuffer, nullptr);
    glCompileShader(m_vertexShader);

    ValidateShader(m_vertexShader);

    m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *fragmentShaderCodeBuffer = PointCloudFragmentShader;
    glShaderSource(m_fragmentShader, 1, &fragmentShaderCodeBuffer, nullptr);
    glCompileShader(m_fragmentShader);
    ValidateShader(m_fragmentShader);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, m_vertexShader);
    glAttachShader(m_shaderProgram, m_fragmentShader);
    glLinkProgram(m_shaderProgram);
    ValidateProgram(m_shaderProgram);

    glGenVertexArrays(1, &m_vertexArrayObject);
    glBindVertexArray(m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);

    m_viewIndex = glGetUniformLocation(m_shaderProgram, "view");
    m_projectionIndex = glGetUniformLocation(m_shaderProgram, "projection");
    m_enableShadingIndex = glGetUniformLocation(m_shaderProgram, "enableShading");
}

PointCloudRenderer::~PointCloudRenderer()
{
    glDeleteBuffers(1, &m_vertexBufferObject);

    glDeleteShader(m_vertexShader);
    glDeleteShader(m_fragmentShader);
    glDeleteProgram(m_shaderProgram);
}

void PointCloudRenderer::UpdateViewProjection(mat4x4 view, mat4x4 projection)
{
    mat4x4_dup(m_view, view);
    mat4x4_dup(m_projection, projection);
}

void PointCloudRenderer::UpdatePointClouds(Vertex *vertices, const unsigned int numVertices)
{
    glBindVertexArray(m_vertexArrayObject);

    // Create buffers and bind the geometry
    //
    if (static_cast<GLsizei>(numVertices) > m_vertexArraySizeMax || m_vertexArraySize == 0)
    {
        ReservePointCloudBuffer(static_cast<GLsizei>(numVertices));
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * sizeof(Vertex), vertices);

    // Set the vertex attribute pointers
    //
    // Vertex Positions
    //
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

    // Vertex Colors
    //
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Color));

    // Vertex Neighbors
    //
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Neighbor));

    glBindVertexArray(0);

    m_vertexArraySize = GLsizei(numVertices);
}

void PointCloudRenderer::ReservePointCloudBuffer(GLsizei numVertices)
{
    GLsizeiptr bufferSize = static_cast<GLsizeiptr>(static_cast<size_t>(numVertices) * sizeof(Vertex));
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_STREAM_DRAW);
    m_vertexArraySize = numVertices;
    m_vertexArraySizeMax = std::max(m_vertexArraySizeMax, m_vertexArraySize);
}

void PointCloudRenderer::Render()
{
    // Save last shader
    //
    GLint lastShader;
    glGetIntegerv(GL_CURRENT_PROGRAM, &lastShader);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(m_pointCloudSize);

    glUseProgram(m_shaderProgram);

    // Update view/projective matrices in shader
    //
    glUniformMatrix4fv(m_viewIndex, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(m_view));
    glUniformMatrix4fv(m_projectionIndex, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(m_projection));

    // Update render settings in shader
    //
    glUniform1i(m_enableShadingIndex, static_cast<GLint>(m_enableShading));

    // Render point cloud
    //
    glBindVertexArray(m_vertexArrayObject);
    glDrawArrays(GL_POINTS, 0, m_vertexArraySize);

    glBindVertexArray(0);

    // Restore shader
    //
    glUseProgram(static_cast<GLuint>(lastShader));
}

void PointCloudRenderer::ChangePointCloudSize(const float pointCloudSize)
{
    m_pointCloudSize = pointCloudSize;
}

void PointCloudRenderer::EnableShading(bool enableShading)
{
    m_enableShading = enableShading;
}

bool PointCloudRenderer::ShadingIsEnabled() const
{
    return m_enableShading;
}
