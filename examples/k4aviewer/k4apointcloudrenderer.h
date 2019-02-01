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

// Library headers
//
#include "k4aimgui_all.h"
#include "linmath.h"

// Project headers
//
#include "k4apointcloudtypes.h"

namespace k4aviewer
{
class PointCloudRenderer
{
public:
    PointCloudRenderer();
    ~PointCloudRenderer();

    void UpdateViewProjection(linmath::mat4x4 view, linmath::mat4x4 projection);

    // Hint to the point cloud renderer what the greatest possible number of points it might receive is
    // to avoid extra OpenGL buffer allocations
    //
    void ReservePointCloudBuffer(GLsizei numPoints);

    void UpdatePointClouds(Vertex* vertices, unsigned int numVertices);

    void Render();

    void ChangePointCloudSize(float pointCloudSize);
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
    GLfloat m_pointCloudSize = 3.f;
    bool m_enableShading = true;

    // Point Array Size
    GLsizei m_vertexArraySize = 0;
    GLsizei m_vertexArraySizeMax = 0;

    // OpenGL resources
    GLuint m_shaderProgram;
    GLuint m_vertexShader;
    GLuint m_fragmentShader;

    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;

    GLuint m_viewIndex;
    GLuint m_projectionIndex;
    GLuint m_enableShadingIndex;
};
} // namespace k4aviewer
#endif
