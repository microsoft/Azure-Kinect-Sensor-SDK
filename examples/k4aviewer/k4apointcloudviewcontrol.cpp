// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4apointcloudviewcontrol.h"

// System headers
//

// Library headers
//

// Project headers
//

using namespace k4aviewer;
using namespace linmath;

namespace
{
inline float Radians(const float angle)
{
    constexpr float pi = 3.14159265358979323846f;
    return angle / 180.f * pi;
}

// Default camera values
const float DefaultSpeed = 2.5f;
const float DefaultSensitivity = 0.1f;
const float DefaultZoom = 65.0f;

// clang-format off
const ViewParameters DefaultView(0.15f, 0.f, -1.25f, // Position
                                 0.f, 1.f, 0.f, // WorldUp
                                 -268.f, 0.f); // Yaw and Pitch
// clang-format on
} // namespace

ViewParameters::ViewParameters(const float posX,
                               const float posY,
                               const float posZ,
                               const float upX,
                               const float upY,
                               const float upZ,
                               const float yaw,
                               const float pitch)
{
    vec3_set(Front, 0.f, 0.f, -1.f);
    vec3_set(Position, posX, posY, posZ);
    vec3_set(WorldUp, upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    UpdateRotationVectors();
}

// Update the rotation vectors based on the updated yaw and pitch values
// It needs to be called every time after updating the yaw and pitch value
void ViewParameters::UpdateRotationVectors()
{
    // Calculate the new m_viewParams.front vector
    vec3 frontTemp;
    frontTemp[0] = static_cast<float>(std::cos(Radians(Yaw)) * std::cos(Radians(Pitch)));
    frontTemp[1] = static_cast<float>(std::sin(Radians(Pitch)));
    frontTemp[2] = static_cast<float>(std::sin(Radians(Yaw)) * std::cos(Radians(Pitch)));
    vec3_norm(Front, frontTemp);

    // Also re-calculate the Right and Up vector
    vec3 rightTemp;
    vec3_mul_cross(rightTemp, Front, WorldUp);
    vec3_norm(Right, rightTemp); // Normalize the vectors, because their length gets closer to 0 the more you look up or
                                 // down which results in slower movement.

    vec3 upTemp;
    vec3_mul_cross(upTemp, Right, Front);
    vec3_norm(Up, upTemp);
}

ViewControl::ViewControl() :
    m_viewParams(DefaultView),
    m_movementSpeed(DefaultSpeed),
    m_mouseSensitivity(DefaultSensitivity),
    m_zoom(DefaultZoom)
{
    ResetPosition();
}

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
void ViewControl::GetViewMatrix(mat4x4 viewMatrix)
{
    vec3 temp;
    vec3_add(temp, m_viewParams.Position, m_viewParams.Front);
    mat4x4_look_at(viewMatrix, m_viewParams.Position, temp, m_viewParams.Up);
}

void ViewControl::GetPerspectiveMatrix(mat4x4 perspectiveMatrix, const int windowWidth, const int windowHeight) const
{
    mat4x4_perspective(perspectiveMatrix, Radians(m_zoom), windowWidth / static_cast<float>(windowHeight), 0.1f, 100.f);
}

// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined
// ENUM (to abstract it from windowing systems)
void ViewControl::ProcessPositionalMovement(const ViewMovement direction, const float deltaTime)
{
    const float velocity = m_movementSpeed * deltaTime;
    vec3 temp;
    switch (direction)
    {
    case ViewMovement::Forward:
        vec3_scale(temp, m_viewParams.Front, velocity);
        vec3_add(m_viewParams.Position, m_viewParams.Position, temp);
        break;
    case ViewMovement::Backward:
        vec3_scale(temp, m_viewParams.Front, velocity);
        vec3_sub(m_viewParams.Position, m_viewParams.Position, temp);
        break;
    case ViewMovement::Left:
        vec3_scale(temp, m_viewParams.Right, velocity);
        vec3_sub(m_viewParams.Position, m_viewParams.Position, temp);
        break;
    case ViewMovement::Right:
        vec3_scale(temp, m_viewParams.Right, velocity);
        vec3_add(m_viewParams.Position, m_viewParams.Position, temp);
        break;
    case ViewMovement::Up:
        vec3_scale(temp, m_viewParams.Up, velocity);
        vec3_add(m_viewParams.Position, m_viewParams.Position, temp);
        break;
    case ViewMovement::Down:
        vec3_scale(temp, m_viewParams.Up, velocity);
        vec3_sub(m_viewParams.Position, m_viewParams.Position, temp);
        break;
    default:
        break;
    }
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void ViewControl::ProcessMouseMovement(float xoffset, float yoffset, const GLboolean constrainPitch)
{
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_viewParams.Yaw += xoffset;
    m_viewParams.Pitch += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (m_viewParams.Pitch > 89.0f)
        {
            m_viewParams.Pitch = 89.0f;
        }
        if (m_viewParams.Pitch < -89.0f)
        {
            m_viewParams.Pitch = -89.0f;
        }
    }

    // Update m_viewParams.front, right and up Vectors using the updated Euler angles
    m_viewParams.UpdateRotationVectors();
}

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void ViewControl::ProcessMouseScroll(const float yoffset)
{
    if (m_zoom >= 1.0f && m_zoom <= 120.0f)
    {
        m_zoom -= yoffset;
    }
    if (m_zoom <= 1.0f)
    {
        m_zoom = 1.0f;
    }
    if (m_zoom >= 120.0f)
    {
        m_zoom = 120.0f;
    }
}

void ViewControl::ResetPosition()
{
    m_viewParams = DefaultView;
    m_zoom = DefaultZoom;
}
