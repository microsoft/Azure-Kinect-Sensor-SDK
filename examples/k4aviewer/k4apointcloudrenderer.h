/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4APOINTCLOUDRENDERER_H
#define K4APOINTCLOUDRENDERER_H

// System headers
//
#include <mutex>

// Library headers
//
#include "k4aimgui_all.h"
#include "linmath.h"

// Project headers
//

namespace k4aviewer
{
class PointCloudRenderer
{
public:
    PointCloudRenderer();
    ~PointCloudRenderer();

    void UpdateModelViewProjection(linmath::mat4x4 model, linmath::mat4x4 view, linmath::mat4x4 projection);

    // Hint to the point cloud rendererwhat the greatest possible number of points it might receive is
    // to avoid extra OpenGL buffer allocations
    //
    void ReservePointCloudBuffer(GLsizei numPoints);

    void UpdatePointClouds(float *pointCoordinates, float *pointColors, unsigned int numPoints);

    void Render();

    void ChangePointCloudSize(float pointCloudSize);

    PointCloudRenderer(PointCloudRenderer &) = delete;
    PointCloudRenderer(PointCloudRenderer &&) = delete;
    PointCloudRenderer &operator=(PointCloudRenderer &) = delete;
    PointCloudRenderer &operator=(PointCloudRenderer &&) = delete;

private:
    linmath::mat4x4 m_model;
    linmath::mat4x4 m_view;
    linmath::mat4x4 m_projection;

    // Render settings
    GLfloat m_pointCloudSize = 3.f;

    // Point Array Size
    GLsizei m_drawArraySize = 0;
    GLsizei m_drawArraySizeMax = 0;

    // OpenGL resources
    GLuint m_shaderProgram;
    GLuint m_vertexShader;
    GLuint m_fragmentShader;

    GLuint m_vertexPositionIndex;
    GLuint m_vertexPositionBuffer;
    GLuint m_vertexColorIndex;
    GLuint m_vertexColorBuffer;
    GLint m_modelIndex;
    GLint m_viewIndex;
    GLint m_projectionIndex;

    GLuint m_vertexAttribArray;

    // Lock
    std::mutex m_mutex;
};
} // namespace k4aviewer
#endif
