// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4APOINTCLOUDVIEWCONTROL_H
#define K4APOINTCLOUDVIEWCONTROL_H

// System headers
//

// Library headers
//
#include "k4aimgui_all.h"
#include "linmath.h"

// Project headers
//

namespace k4aviewer
{

// The type of movement that a mouse movement should be interpreted as (if any)
//
enum class MouseMovementType
{
    None,
    Rotation,
    Translation
};

// A camera class that processes input and calculates the view matrices for use in OpenGL
//
class ViewControl
{
public:
    ViewControl();

    // Returns the view matrix calculated using an arcball camera
    //
    void GetViewMatrix(linmath::mat4x4 viewMatrix);

    // Gets a matrix representing the perspective transformation
    //
    void GetPerspectiveMatrix(linmath::mat4x4 perspectiveMatrix, const linmath::vec2 renderDimensions) const;

    // Processes input received from a mouse input system.
    // Mouse position is relative to the start of the point cloud display, not the window.
    //
    void ProcessMouseMovement(const linmath::vec2 displayDimensions,
                              const linmath::vec2 mousePos,
                              const linmath::vec2 mouseDelta,
                              MouseMovementType movementType);

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    //
    void ProcessMouseScroll(float yoffset);

    // Reset camera view back to default position
    //
    void ResetPosition();

private:
    float m_zoom;
    linmath::mat4x4 m_userRotations;
    linmath::vec3 m_pointCloudPosition;
};
} // namespace k4aviewer

#endif
