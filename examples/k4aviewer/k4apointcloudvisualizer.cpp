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
// TODO enable adjustable dimensions
//
constexpr ImageDimensions PointCloudVisualizerTextureDimensions = { 640, 576 };

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

    m_pointCloudRenderer.UpdateModelViewProjection(m_model, m_view, m_projection);
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

    linmath::mat4x4_identity(m_model);
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
    if (m_depthPositionBuffer.size() < pointCount)
    {
        m_depthPositionBuffer.resize(pointCount);
        m_depthColorBuffer.resize(pointCount);
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
    for (size_t srcIndex = 0; srcIndex < pointCount; ++srcIndex)
    {
        k4a_float3_t depthPoint3D = m_depthPositionBuffer[srcIndex];
        depthPoint3D.xyz.z = static_cast<float>(pointCloudBuffer[3 * srcIndex + 2]);
        if (depthPoint3D.xyz.z <= 0.f)
        {
            continue;
        }

        // OpenGL uses a different coordinate system than k4a's xrydzf. We must flip the X coordinate.
        //
        depthPoint3D.xyz.x = float(-1 * pointCloudBuffer[3 * srcIndex]);
        depthPoint3D.xyz.y = float(pointCloudBuffer[3 * srcIndex + 1]);

        // Downscale coordinates to fit the camera FOV
        //
        // TODO consider scaling the camera to the coordinates rather than the coordinates to the camera
        //
        for (float &f : depthPoint3D.v)
        {
            f /= 1000.0f;
        }
        m_depthPositionBuffer[dstIndex] = depthPoint3D;

        // TODO integrate color from RGB camera
        //
        const RgbPixel colorization =
            K4ADepthPixelColorizer::ColorizeRedToBlue(m_expectedValueRange,
                                                      static_cast<uint16_t>(pointCloudBuffer[3 * srcIndex + 2]));
        constexpr auto uint8_t_max = static_cast<float>(std::numeric_limits<uint8_t>::max());
        m_depthColorBuffer[dstIndex].xyz.x = colorization.Red / uint8_t_max;
        m_depthColorBuffer[dstIndex].xyz.y = colorization.Green / uint8_t_max;
        m_depthColorBuffer[dstIndex].xyz.z = colorization.Blue / uint8_t_max;

        ++dstIndex;
    }

    if (!m_pointCloudRendererBufferInitialized)
    {
        m_pointCloudRenderer.ReservePointCloudBuffer(static_cast<GLsizei>(pointCount));
        m_pointCloudRendererBufferInitialized = true;
    }

    m_pointCloudRenderer.UpdatePointClouds(reinterpret_cast<float *>(&m_depthPositionBuffer[0]),
                                           reinterpret_cast<float *>(&m_depthColorBuffer[0]),
                                           static_cast<unsigned int>(dstIndex));

    k4a_image_release(depthImage);
}
