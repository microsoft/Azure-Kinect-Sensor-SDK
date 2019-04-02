// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4apointcloudviewcontrol.h"

// System headers
//
#include <algorithm>

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
//
constexpr float MinZoom = 1.0f;
constexpr float MaxZoom = 120.0f;
constexpr float DefaultZoom = 65.0f;
constexpr float ZoomSensitivity = 3.f;
constexpr float TranslationSensitivity = 0.01f;

// Default point cloud position, chosen such that the entire point cloud
// should be in the field of view
//
const vec3 DefaultPointCloudPosition{ 0.f, 0.f, -8.0f };

// Approximate midpoint of a point cloud.  We need to translate
// the point cloud by this much before we start applying rotations to the
// point cloud in order to to get the rotation to be around the point
// cloud's midpoint instead of its origin.
//
const vec3 PointCloudMidpoint{ 0.f, 1.f, -3.0f };

// Version of mat4x4_mul that doesn't modify its non-result inputs (i.e. a, b)
//
void MatrixMultiply(mat4x4 out, mat4x4 a, mat4x4 b)
{
    mat4x4 atmp;
    mat4x4 btmp;
    mat4x4_dup(atmp, a);
    mat4x4_dup(btmp, b);
    mat4x4_mul(out, a, b);
}

// Map XY coordinates to a virtual sphere, which we use for rotation calculations
//
void MapToArcball(vec3 out, const vec2 displayDimensions, const vec2 mousePos)
{
    // Scale coords to (-1, 1) to simplify some of the math
    //
    vec2 scaledMousePos;
    for (int i = 0; i < 2; ++i)
    {
        scaledMousePos[i] = mousePos[i] * (1.0f / ((displayDimensions[i] - 1.0f) * 0.5f)) - 1.0f;
    }

    float lenSquared = scaledMousePos[0] * scaledMousePos[0] + scaledMousePos[1] * scaledMousePos[1];

    // If the point is 'outside' our virtual sphere, we need to normalize to the sphere
    // This works because our sphere is of radius 1
    //
    if (lenSquared > 1.f)
    {
        const float normalizationFactor = 1.f / std::sqrt(lenSquared);

        // Return a point on the edge of the sphere
        //
        out[0] = scaledMousePos[0] * normalizationFactor;
        out[1] = scaledMousePos[1] * normalizationFactor;
        out[2] = 0.f;
    }
    else
    {
        // Return a point inside the sphere
        //
        out[0] = scaledMousePos[0];
        out[1] = scaledMousePos[1];
        out[2] = std::sqrt(1.f - lenSquared);
    }
}

// Imagine a virtual sphere of displayDimensions size were drawn on the screen.
// Returns a quaternion representing the rotation that sphere would undergo if you took the point
// on that sphere at startPos and rotated it to endPos (i.e. an "arcball" camera).
//
void GetArcballRotation(quat rotation, const vec2 displayDimensions, const vec2 startPos, const vec2 endPos)
{
    vec3 startVector;
    MapToArcball(startVector, displayDimensions, startPos);

    vec3 endVector;
    MapToArcball(endVector, displayDimensions, endPos);

    vec3 cross;
    vec3_mul_cross(cross, startVector, endVector);

    constexpr float epsilon = 0.001f;
    if (vec3_len(cross) < epsilon)
    {
        // Smooth out floating point error if the user didn't move the mouse
        // enough that it should register
        //
        quat_identity(rotation);
    }
    else
    {
        // The first 3 elements of the quaternion are the unit vector perpendicular
        // to the rotation (i.e. the cross product); the last element is the magnitude
        // of the rotation
        //
        vec3_copy(rotation, cross);
        vec3_norm(cross, cross);
        rotation[3] = vec3_mul_inner(startVector, endVector);
    }
}
} // namespace

ViewControl::ViewControl() : m_zoom(DefaultZoom)
{
    ResetPosition();
}

void ViewControl::GetViewMatrix(mat4x4 viewMatrix)
{
    mat4x4_identity(viewMatrix);

    // Move the center of the point cloud to (0, 0, 0) so we can rotate it
    //
    mat4x4 pointCloudMidpointTranslation;
    mat4x4_translate(pointCloudMidpointTranslation,
                     PointCloudMidpoint[0],
                     PointCloudMidpoint[1],
                     PointCloudMidpoint[2]);

    // Move the point cloud to a point in front of the field of view
    //
    mat4x4 pointCloudFinalTranslation;
    mat4x4_translate(pointCloudFinalTranslation,
                     m_pointCloudPosition[0],
                     m_pointCloudPosition[1],
                     m_pointCloudPosition[2]);

    // Rotate 180 degrees about the Y axis so the scene starts out facing toward the user
    //
    quat rotateQuat;
    vec3 rotateAxis{ 0.f, 1.f, 0.f };
    mat4x4 rotateMatrix;
    quat_rotate(rotateQuat, Radians(180), rotateAxis);
    mat4x4_from_quat(rotateMatrix, rotateQuat);

    // Multiplication order is reversed because we're moving the scene, not the camera
    //

    // Move the point cloud into the field of view
    //
    MatrixMultiply(viewMatrix, viewMatrix, pointCloudFinalTranslation);

    // Set up the point cloud
    //
    MatrixMultiply(viewMatrix, viewMatrix, m_userRotations);
    MatrixMultiply(viewMatrix, viewMatrix, rotateMatrix);
    MatrixMultiply(viewMatrix, viewMatrix, pointCloudMidpointTranslation);
}

void ViewControl::GetPerspectiveMatrix(mat4x4 perspectiveMatrix, const vec2 renderDimensions) const
{
    mat4x4_perspective(perspectiveMatrix, Radians(m_zoom), renderDimensions[0] / renderDimensions[1], 0.1f, 100.f);
}

void ViewControl::ProcessMouseMovement(const vec2 displayDimensions,
                                       const vec2 mousePos,
                                       const vec2 mouseDelta,
                                       MouseMovementType movementType)
{
    if (movementType == MouseMovementType::Rotation)
    {
        vec2 lastMousePos;
        vec2_copy(lastMousePos, mousePos);
        vec2_sub(lastMousePos, mousePos, mouseDelta);

        quat newRotationQuat;
        GetArcballRotation(newRotationQuat, displayDimensions, lastMousePos, mousePos);

        mat4x4 newRotationMtx;
        mat4x4_from_quat(newRotationMtx, newRotationQuat);

        MatrixMultiply(m_userRotations, newRotationMtx, m_userRotations);
    }
    else if (movementType == MouseMovementType::Translation)
    {
        m_pointCloudPosition[0] += mouseDelta[0] * TranslationSensitivity;
        m_pointCloudPosition[1] += mouseDelta[1] * TranslationSensitivity;
    }
}

void ViewControl::ProcessMouseScroll(const float yoffset)
{
    if (m_zoom >= MinZoom && m_zoom <= MaxZoom)
    {
        m_zoom -= yoffset * ZoomSensitivity;
        m_zoom = std::min(MaxZoom, std::max(MinZoom, m_zoom));
    }
}

void ViewControl::ResetPosition()
{
    vec3_copy(m_pointCloudPosition, DefaultPointCloudPosition);
    m_zoom = DefaultZoom;
    mat4x4_identity(m_userRotations);
}
