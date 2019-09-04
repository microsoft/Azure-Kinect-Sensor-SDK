// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
#include "k4acolorimageconverter.h"
#include "k4adepthpixelcolorizer.h"
#include "k4astaticimageproperties.h"
#include "k4aviewerutil.h"
#include "perfcounter.h"

using namespace k4aviewer;

namespace
{
// Background color of point cloud viewer - dark grey
//
const ImVec4 ClearColor = { 0.05f, 0.05f, 0.05f, 0.0f };

// Resolution of the point cloud texture
//
constexpr ImageDimensions PointCloudVisualizerTextureDimensions = { 1280, 1152 };

} // namespace

GLenum K4APointCloudVisualizer::InitializeTexture(std::shared_ptr<K4AViewerImage> *texture) const
{
    return K4AViewerImage::Create(texture, nullptr, m_dimensions, GL_RGBA);
}

PointCloudVisualizationResult K4APointCloudVisualizer::UpdateTexture(std::shared_ptr<K4AViewerImage> *texture,
                                                                     const k4a::capture &capture)
{
    // Update the point cloud renderer with the latest point data
    //
    PointCloudVisualizationResult result = UpdatePointClouds(capture);
    if (result != PointCloudVisualizationResult::Success)
    {
        return result;
    }

    // Set up rendering to a texture
    //
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer.Id());
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer.Id());
    CleanupGuard frameBufferBindingGuard([]() { glBindFramebuffer(GL_FRAMEBUFFER, 0); });

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer.Id());

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, static_cast<GLuint>(**texture), 0);
    const GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffers);

    const GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        return PointCloudVisualizationResult::OpenGlError;
    }

    glViewport(0, 0, m_dimensions.Width, m_dimensions.Height);

    glEnable(GL_DEPTH_TEST);
    glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
    glClearDepth(1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const linmath::vec2 displayDimensions{ static_cast<float>(m_dimensions.Width),
                                           static_cast<float>(m_dimensions.Height) };
    m_viewControl.GetPerspectiveMatrix(m_projection, displayDimensions);
    m_viewControl.GetViewMatrix(m_view);

    m_pointCloudRenderer.UpdateViewProjection(m_view, m_projection);

    GLenum renderStatus = m_pointCloudRenderer.Render();

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    if (renderStatus != GL_NO_ERROR)
    {
        return PointCloudVisualizationResult::OpenGlError;
    }

    return PointCloudVisualizationResult::Success;
}

void K4APointCloudVisualizer::ProcessMouseMovement(const linmath::vec2 displayDimensions,
                                                   const linmath::vec2 mousePos,
                                                   const linmath::vec2 mouseDelta,
                                                   MouseMovementType movementType)
{
    m_viewControl.ProcessMouseMovement(displayDimensions, mousePos, mouseDelta, movementType);
}

void K4APointCloudVisualizer::ProcessMouseScroll(const float yoffset)
{
    m_viewControl.ProcessMouseScroll(yoffset);
}

void K4APointCloudVisualizer::ResetPosition()
{
    m_viewControl.ResetPosition();
}

PointCloudVisualizationResult K4APointCloudVisualizer::SetColorizationStrategy(ColorizationStrategy strategy)
{
    if (strategy == ColorizationStrategy::Color && !m_enableColorPointCloud)
    {
        throw std::logic_error("Attempted to set unsupported point cloud mode!");
    }

    m_colorizationStrategy = strategy;

    m_pointCloudRenderer.EnableShading(m_colorizationStrategy == ColorizationStrategy::Shaded);

    GLenum xyTableStatus = GL_NO_ERROR;
    if (m_colorizationStrategy == ColorizationStrategy::Color)
    {
        m_transformedDepthImage = k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16,
                                                     m_calibrationData.color_camera_calibration.resolution_width,
                                                     m_calibrationData.color_camera_calibration.resolution_height,
                                                     m_calibrationData.color_camera_calibration.resolution_width *
                                                         static_cast<int>(sizeof(DepthPixel)));

        xyTableStatus = m_pointCloudConverter.SetActiveXyTable(m_colorXyTable);
    }
    else
    {

        m_pointCloudColorization = k4a::image::create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                      m_calibrationData.depth_camera_calibration.resolution_width,
                                                      m_calibrationData.depth_camera_calibration.resolution_height,
                                                      m_calibrationData.depth_camera_calibration.resolution_width *
                                                          static_cast<int>(sizeof(BgraPixel)));

        xyTableStatus = m_pointCloudConverter.SetActiveXyTable(m_depthXyTable);
    }

    if (xyTableStatus != GL_NO_ERROR)
    {
        return PointCloudVisualizationResult::OpenGlError;
    }

    // Reset our reserved XYZ point cloud texture so it'll get resized the next time we go to render
    //
    m_xyzTexture.Reset();

    // If we've had data, force-refresh color pixels uploaded to GPU.
    // This allows us to switch shading modes while paused.
    //
    if (m_lastCapture)
    {
        return UpdatePointClouds(m_lastCapture);
    }

    return PointCloudVisualizationResult::Success;
}

void K4APointCloudVisualizer::SetPointSize(int size)
{
    m_pointCloudRenderer.SetPointSize(size);
}

K4APointCloudVisualizer::K4APointCloudVisualizer(const bool enableColorPointCloud,
                                                 const k4a::calibration &calibrationData) :
    m_dimensions(PointCloudVisualizerTextureDimensions),
    m_enableColorPointCloud(enableColorPointCloud),
    m_calibrationData(calibrationData)
{
    m_expectedValueRange = GetDepthModeRange(m_calibrationData.depth_mode);
    m_transformation = k4a::transformation(m_calibrationData);

    glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer.Id());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_dimensions.Width, m_dimensions.Height);

    linmath::mat4x4_identity(m_view);
    linmath::mat4x4_identity(m_projection);

    m_viewControl.ResetPosition();

    if (enableColorPointCloud)
    {
        m_colorXyTable = m_pointCloudConverter.GenerateXyTable(m_calibrationData, K4A_CALIBRATION_TYPE_COLOR);
    }

    m_depthXyTable = m_pointCloudConverter.GenerateXyTable(m_calibrationData, K4A_CALIBRATION_TYPE_DEPTH);

    SetColorizationStrategy(m_colorizationStrategy);
}

PointCloudVisualizationResult K4APointCloudVisualizer::UpdatePointClouds(const k4a::capture &capture)
{
    k4a::image depthImage = capture.get_depth_image();
    if (!depthImage)
    {
        // Capture doesn't have depth info; drop the capture
        //
        return PointCloudVisualizationResult::MissingDepthImage;
    }

    k4a::image colorImage = capture.get_color_image();

    if (m_enableColorPointCloud)
    {
        if (!colorImage)
        {
            // Capture doesn't have color info; drop the capture
            //
            return PointCloudVisualizationResult::MissingColorImage;
        }

        if (m_colorizationStrategy == ColorizationStrategy::Color)
        {
            try
            {
                m_transformation.depth_image_to_color_camera(depthImage, &m_transformedDepthImage);
                depthImage = m_transformedDepthImage;
            }
            catch (const k4a::error &)
            {
                return PointCloudVisualizationResult::DepthToColorTransformationFailed;
            }
        }
    }

    GLenum glResult = m_pointCloudConverter.Convert(depthImage, &m_xyzTexture);
    if (glResult != GL_NO_ERROR)
    {
        return PointCloudVisualizationResult::DepthToXyzTransformationFailed;
    }

    m_lastCapture = capture;

    if (m_colorizationStrategy == ColorizationStrategy::Color)
    {
        m_pointCloudColorization = std::move(colorImage);
    }
    else
    {
        DepthPixel *srcPixel = reinterpret_cast<DepthPixel *>(depthImage.get_buffer());
        BgraPixel *dstPixel = reinterpret_cast<BgraPixel *>(m_pointCloudColorization.get_buffer());
        const BgraPixel *endPixel = dstPixel + (depthImage.get_size() / sizeof(DepthPixel));

        while (dstPixel != endPixel)
        {
            *dstPixel = K4ADepthPixelColorizer::ColorizeBlueToRed(*srcPixel,
                                                                  m_expectedValueRange.first,
                                                                  m_expectedValueRange.second);

            ++dstPixel;
            ++srcPixel;
        }
    }

    GLenum updatePointCloudResult = m_pointCloudRenderer.UpdatePointClouds(m_pointCloudColorization, m_xyzTexture);
    if (updatePointCloudResult != GL_NO_ERROR)
    {
        return PointCloudVisualizationResult::OpenGlError;
    }

    return PointCloudVisualizationResult::Success;
}
