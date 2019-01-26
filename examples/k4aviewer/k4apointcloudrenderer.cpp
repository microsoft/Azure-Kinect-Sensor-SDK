/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4apointcloudrenderer.h"

// System headers
//
#include <thread>
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
    mat4x4_identity(m_model);
    mat4x4_identity(m_view);
    mat4x4_identity(m_projection);

    // Context Settings
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(m_pointCloudSize);

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

    glGenVertexArrays(1, &m_vertexAttribArray);

    m_vertexPositionIndex = static_cast<GLuint>(glGetAttribLocation(m_shaderProgram, "vertex_position"));
    m_vertexColorIndex = static_cast<GLuint>(glGetAttribLocation(m_shaderProgram, "vertex_color"));
    m_modelIndex = glGetUniformLocation(m_shaderProgram, "model");
    m_viewIndex = glGetUniformLocation(m_shaderProgram, "view");
    m_projectionIndex = glGetUniformLocation(m_shaderProgram, "projection");

    // Generate Vertex Buffer Indices
    glGenBuffers(1, &m_vertexPositionBuffer);
    glGenBuffers(1, &m_vertexColorBuffer);
}

PointCloudRenderer::~PointCloudRenderer()
{
    glDeleteBuffers(1, &m_vertexPositionBuffer);
    glDeleteBuffers(1, &m_vertexColorBuffer);

    glDeleteShader(m_vertexShader);
    glDeleteShader(m_fragmentShader);
    glDeleteProgram(m_shaderProgram);
}

void PointCloudRenderer::UpdateModelViewProjection(mat4x4 model, mat4x4 view, mat4x4 projection)
{
    mat4x4_dup(m_model, model);
    mat4x4_dup(m_view, view);
    mat4x4_dup(m_projection, projection);
}

void PointCloudRenderer::UpdatePointClouds(float *pointCoordinates, float *pointColors, const unsigned int numPoints)
{
    m_drawArraySize = GLsizei(numPoints);

    const auto pointBufferSizeBytes = static_cast<GLsizeiptr>(m_drawArraySize * GLsizei(3) * GLsizei(sizeof(float)));

    // Upload point cloud data to the GPU
    //
    if (m_drawArraySize > m_drawArraySizeMax || m_drawArraySize == 0)
    {
        ReservePointCloudBuffer(static_cast<GLsizei>(numPoints));
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexPositionBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pointBufferSizeBytes, pointCoordinates);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexColorBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pointBufferSizeBytes, pointColors);
}

void PointCloudRenderer::ReservePointCloudBuffer(GLsizei numPoints)
{
    const auto pointBufferSizeBytes = static_cast<GLsizei>(numPoints * GLsizei(3) * GLsizei(sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, pointBufferSizeBytes, nullptr, GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, pointBufferSizeBytes, nullptr, GL_STREAM_DRAW);

    m_drawArraySizeMax = pointBufferSizeBytes;
}

void PointCloudRenderer::Render()
{
    // Save last shader
    GLint lastShader;
    glGetIntegerv(GL_CURRENT_PROGRAM, &lastShader);

    glUseProgram(m_shaderProgram);

    // Update model/view/projective matrices in shader
    glUniformMatrix4fv(m_modelIndex, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(m_model));
    glUniformMatrix4fv(m_viewIndex, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(m_view));
    glUniformMatrix4fv(m_projectionIndex, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(m_projection));

    // Update point cloud information in shader
    glBindVertexArray(m_vertexAttribArray);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexPositionBuffer);
    glVertexAttribPointer(m_vertexPositionIndex, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(m_vertexPositionIndex);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexColorBuffer);
    glVertexAttribPointer(m_vertexColorIndex, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(m_vertexColorIndex);

    // Render point cloud
    glDrawArrays(GL_POINTS, 0, m_drawArraySize);
    glDisableVertexAttribArray(m_vertexPositionIndex);
    glDisableVertexAttribArray(m_vertexColorIndex);

    // Restore shader
    glUseProgram(static_cast<GLuint>(lastShader));
}

void PointCloudRenderer::ChangePointCloudSize(const float pointCloudSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_pointCloudSize = pointCloudSize;
}
