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
// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific
// input methods
enum class ViewMovement
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

struct ViewParameters
{
    ViewParameters(const ViewParameters &v) = default;
    ViewParameters &operator=(const ViewParameters &v) = default;

    ViewParameters(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    // Update the rotation vectors based on the updated yaw and pitch values
    // It needs to be called every time after updating the yaw and pitch value
    void UpdateRotationVectors();

    // Camera Attributes
    linmath::vec3 Position;
    linmath::vec3 Front;
    linmath::vec3 Up;
    linmath::vec3 Right;
    linmath::vec3 WorldUp;

    // Euler Angles
    float Yaw;
    float Pitch;
};

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for
// use in OpenGL
class ViewControl
{
public:
    // Constructor with scalar values
    ViewControl();

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    void GetViewMatrix(linmath::mat4x4 viewMatrix);

    void GetPerspectiveMatrix(linmath::mat4x4 perspectiveMatrix, int windowWidth, int windowHeight) const;

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera
    // defined ENUM (to abstract it from windowing systems)
    void ProcessPositionalMovement(ViewMovement direction, float deltaTime);

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);

    // Reset camera view back to default position
    void ResetPosition();

private:
    ViewParameters m_viewParams;

    // Camera options
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;
};
} // namespace k4aviewer

#endif
