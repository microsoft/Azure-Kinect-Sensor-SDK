/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4apointcloudvisualizer.h"

// System headers
//

// Library headers
//
#include <ratio>

// Project headers
//
#include "assertionexception.h"
#include "k4adepthpixelcolorizer.h"
#include "k4aviewerutil.h"

using namespace k4aviewer;

namespace
{
// Background color of point cloud viewer - dark grey
//
const ImVec4 ClearColor = { 0.05f, 0.05f, 0.05f, 0.0f };

// Resolution of the point cloud texture
//
constexpr ImageDimensions PointCloudVisualizerTextureDimensions = { 640, 576 };

inline k4a_float3_t ConvertK4AToOpenGLCoordinate(k4a_float3_t pt)
{
    // OpenGL and K4A have different conventions on which direction is positive -
    // we need to flip the X coordinate.
    //
    return { { -pt.v[0], pt.v[1], pt.v[2] } };
}

inline void ConvertK4AFloat3ToLinmathVec3(const k4a_float3_t &in, linmath::vec3 &out)
{
    constexpr size_t vecSize = sizeof(in.v) / sizeof(float);
    std::copy(in.v, in.v + vecSize, out);
}

bool GetPoint3d(k4a_float3_t &point3d, int16_t *pointCloudBuffer, int depthW, int depthH, int x, int y)
{
    if (x < 0 || x >= depthW || y < 0 || y >= depthH)
    {
        return false;
    }

    const int pixelOffset = y * depthW + x;
    float zVal = static_cast<float>(pointCloudBuffer[3 * pixelOffset + 2]);
    if (zVal <= 0.0f)
    {
        return false;
    }

    point3d.v[0] = static_cast<float>(pointCloudBuffer[3 * pixelOffset + 0]);
    point3d.v[1] = static_cast<float>(pointCloudBuffer[3 * pixelOffset + 1]);
    point3d.v[2] = zVal;

    point3d = ConvertK4AToOpenGLCoordinate(point3d);

    return true;
}

} // namespace

GLenum K4APointCloudVisualizer::InitializeTexture(std::shared_ptr<OpenGlTexture> &texture) const
{
    return OpenGlTextureFactory::CreateTexture(texture, nullptr, m_dimensions, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
}

GLenum K4APointCloudVisualizer::UpdateTexture(std::shared_ptr<OpenGlTexture> &texture,
                                              const K4AImage<K4A_IMAGE_FORMAT_DEPTH16> &frame)
{
    // Set up rendering to a texture
    //
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    CleanupGuard frameBufferBindingGuard([]() { glBindFramebuffer(GL_FRAMEBUFFER, 0); });

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, static_cast<GLuint>(*texture), 0);
    const GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffers);

    const GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        return frameBufferStatus;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    glViewport(0, 0, m_dimensions.Width, m_dimensions.Height);

    glEnable(GL_DEPTH_TEST);
    glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
    glClearDepth(1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update the point cloud renderer with the latest point data
    //
    UpdatePointClouds(frame);

    m_viewControl.GetPerspectiveMatrix(m_projection, m_dimensions.Width, m_dimensions.Height);
    m_viewControl.GetViewMatrix(m_view);

    m_pointCloudRenderer.UpdateViewProjection(m_view, m_projection);
    m_pointCloudRenderer.Render();

    return glGetError();
}

void K4APointCloudVisualizer::ProcessPositionalMovement(const ViewMovement direction, const float deltaTime)
{
    m_viewControl.ProcessPositionalMovement(direction, deltaTime);
}

void K4APointCloudVisualizer::ProcessMouseMovement(const float xoffset, const float yoffset)
{
    m_viewControl.ProcessMouseMovement(xoffset, yoffset);
}

void K4APointCloudVisualizer::ProcessMouseScroll(const float yoffset)
{
    m_viewControl.ProcessMouseScroll(yoffset);
}

void K4APointCloudVisualizer::ResetPosition()
{
    m_viewControl.ResetPosition();
}

void K4APointCloudVisualizer::EnableShading(bool enable)
{
    m_pointCloudRenderer.EnableShading(enable);
}

K4APointCloudVisualizer::K4APointCloudVisualizer(const k4a_depth_mode_t depthMode,
                                                 std::unique_ptr<K4ACalibrationTransformData> &&calibrationData) :
    m_expectedValueRange(GetRangeForDepthMode(depthMode)),
    m_dimensions(PointCloudVisualizerTextureDimensions),
    m_calibrationTransformData(std::move(calibrationData))
{
    glGenFramebuffers(1, &m_frameBuffer);

    glGenRenderbuffers(1, &m_depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_dimensions.Width, m_dimensions.Height);

    linmath::mat4x4_identity(m_view);
    linmath::mat4x4_identity(m_projection);

    m_viewControl.ResetPosition();
}

K4APointCloudVisualizer::~K4APointCloudVisualizer()
{
    glDeleteFramebuffers(1, &m_frameBuffer);
}

void K4APointCloudVisualizer::UpdatePointClouds(const K4AImage<K4A_IMAGE_FORMAT_DEPTH16> &frame)
{
    const size_t pointCount = frame.GetSize() / sizeof(DepthPixel);
    if (m_vertexBuffer.size() < pointCount)
    {
        m_vertexBuffer.resize(pointCount);
    }

    k4a_image_t depthImage = nullptr;
    k4a_image_create_from_buffer(K4A_IMAGE_FORMAT_DEPTH16,
                                 m_calibrationTransformData->DepthWidth,
                                 m_calibrationTransformData->DepthHeight,
                                 m_calibrationTransformData->DepthWidth * int(sizeof(uint16_t)),
                                 frame.GetBuffer(),
                                 frame.GetSize(),
                                 nullptr,
                                 nullptr,
                                 &depthImage);

    k4a_transformation_depth_image_to_point_cloud(m_calibrationTransformData->TransformationHandle,
                                                  depthImage,
                                                  K4A_CALIBRATION_TYPE_DEPTH,
                                                  m_calibrationTransformData->PointCloudImage);

    // pointCloudBuffer stores interleaved 3d coordinates. To access the x-, y- and z- coordinates of the ith pixel use
    // pointCloudBuffer[3 * i], pointCloudBuffer[3 * i + 1] and pointCloudBuffer[3 * i + 2], respectively.
    //
    auto *pointCloudBuffer = reinterpret_cast<int16_t *>(
        k4a_image_get_buffer(m_calibrationTransformData->PointCloudImage));

    size_t dstIndex = 0;
    const int frameHeight = frame.GetHeightPixels();
    const int frameWidth = frame.GetWidthPixels();
    for (int h = 0; h < frameHeight; ++h)
    {
        for (int w = 0; w < frameWidth; ++w)
        {
            Vertex &outputVertex = m_vertexBuffer[dstIndex];

            k4a_float3_t position;
            if (!GetPoint3d(position, pointCloudBuffer, frameWidth, frameHeight, w, h))
            {
                continue;
            }

            ConvertK4AFloat3ToLinmathVec3(position, outputVertex.Position);

            // Vertex positions are in millimeters, but everything else is in meters, so we need to convert
            //
            for (float &f : outputVertex.Position)
            {
                f /= 1000.0f;
            }

            // Compute color
            //
            const RgbPixel colorization = K4ADepthPixelColorizer::ColorizeRedToBlue(m_expectedValueRange,
                                                                                    static_cast<DepthPixel>(
                                                                                        position.xyz.z));

            constexpr auto uint8_t_max = static_cast<float>(std::numeric_limits<uint8_t>::max());
            outputVertex.Color[0] = colorization.Red / uint8_t_max;
            outputVertex.Color[1] = colorization.Green / uint8_t_max;
            outputVertex.Color[2] = colorization.Blue / uint8_t_max;
            outputVertex.Color[3] = uint8_t_max; // alpha

            // Compute neighbors - only necessary if we're using the fancy shader
            //
            if (m_pointCloudRenderer.ShadingIsEnabled())
            {
                // Find the neighboring point that is closest to the camera
                k4a_float3_t closest = position;
                closest.v[2] += 1000.0f; // If no neighbors have data, default to 1 meter behind point.

                k4a_float3_t pointLeft, pointRight, pointUp, pointDown;
                if (GetPoint3d(pointLeft, pointCloudBuffer, frameWidth, frameHeight, w - 1, h))
                {
                    if (closest.v[2] > pointLeft.v[2])
                    {
                        closest = pointLeft;
                    }
                }
                if (GetPoint3d(pointRight, pointCloudBuffer, frameWidth, frameHeight, w + 1, h))
                {
                    if (closest.v[2] > pointRight.v[2])
                    {
                        closest = pointRight;
                    }
                }
                if (GetPoint3d(pointUp, pointCloudBuffer, frameWidth, frameHeight, w, h - 1))
                {
                    if (closest.v[2] > pointUp.v[2])
                    {
                        closest = pointUp;
                    }
                }
                if (GetPoint3d(pointDown, pointCloudBuffer, frameWidth, frameHeight, w, h + 1))
                {
                    if (closest.v[2] > pointDown.v[2])
                    {
                        closest = pointDown;
                    }
                }

                ConvertK4AFloat3ToLinmathVec3(closest, outputVertex.Neighbor);

                // Vertex positions are in millimeters, but everything else is in meters, so we need to convert
                //
                for (float &f : outputVertex.Neighbor)
                {
                    f /= 1000.0f;
                }
            }

            // Mark point succeeded
            //
            ++dstIndex;
        }
    }

    if (!m_pointCloudRendererBufferInitialized)
    {
        m_pointCloudRenderer.ReservePointCloudBuffer(static_cast<GLsizei>(pointCount));
        m_pointCloudRendererBufferInitialized = true;
    }

    m_pointCloudRenderer.UpdatePointClouds(reinterpret_cast<Vertex *>(&m_vertexBuffer[0]),
                                           static_cast<unsigned int>(dstIndex));

    k4a_image_release(depthImage);
}
