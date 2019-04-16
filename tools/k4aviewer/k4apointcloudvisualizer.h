// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4APOINTCLOUDVISUALIZER_H
#define K4APOINTCLOUDVISUALIZER_H

// System headers
//
#include <vector>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//
#include "gpudepthtopointcloudconverter.h"
#include "k4apointcloudrenderer.h"
#include "k4apointcloudviewcontrol.h"
#include "k4aviewerutil.h"
#include "k4aviewerimage.h"

namespace k4aviewer
{

enum class PointCloudVisualizationResult
{
    Success,
    OpenGlError,
    MissingDepthImage,
    MissingColorImage,
    DepthToXyzTransformationFailed,
    DepthToColorTransformationFailed
};

class K4APointCloudVisualizer
{
public:
    enum class ColorizationStrategy
    {
        Simple,
        Shaded,
        Color
    };

    GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> *texture) const;
    PointCloudVisualizationResult UpdateTexture(std::shared_ptr<K4AViewerImage> *texture, const k4a::capture &capture);

    void ProcessMouseMovement(const linmath::vec2 displayDimensions,
                              const linmath::vec2 mousePos,
                              const linmath::vec2 mouseDelta,
                              MouseMovementType movementType);
    void ProcessMouseScroll(float yoffset);

    void ResetPosition();

    PointCloudVisualizationResult SetColorizationStrategy(ColorizationStrategy strategy);
    void SetPointSize(int size);

    K4APointCloudVisualizer(bool enableColorPointCloud, const k4a::calibration &calibrationData);
    ~K4APointCloudVisualizer() = default;

    K4APointCloudVisualizer(const K4APointCloudVisualizer &) = delete;
    K4APointCloudVisualizer &operator=(const K4APointCloudVisualizer &) = delete;
    K4APointCloudVisualizer(const K4APointCloudVisualizer &&) = delete;
    K4APointCloudVisualizer &operator=(const K4APointCloudVisualizer &&) = delete;

private:
    PointCloudVisualizationResult UpdatePointClouds(const k4a::capture &capture);

    std::pair<DepthPixel, DepthPixel> m_expectedValueRange;
    ImageDimensions m_dimensions;

    PointCloudRenderer m_pointCloudRenderer;
    ViewControl m_viewControl;

    bool m_enableColorPointCloud = false;
    ColorizationStrategy m_colorizationStrategy;

    linmath::mat4x4 m_projection{};
    linmath::mat4x4 m_view{};

    OpenGL::Framebuffer m_frameBuffer = OpenGL::Framebuffer(true);
    OpenGL::Renderbuffer m_depthBuffer = OpenGL::Renderbuffer(true);

    k4a::calibration m_calibrationData;
    k4a::transformation m_transformation;

    k4a::capture m_lastCapture;

    // Buffer that holds the depth image transformed to the color coordinate space.
    // Used in color mode only.
    //
    k4a::image m_transformedDepthImage;

    // In color mode, this is just a shallow copy of the latest color image.
    // In depth mode, this is a buffer that holds the colorization of the depth image.
    //
    k4a::image m_pointCloudColorization;

    // Holds the XYZ point cloud as a texture.
    // Format is XYZA, where A (the alpha channel) is unused.
    //
    OpenGL::Texture m_xyzTexture;

    GpuDepthToPointCloudConverter m_pointCloudConverter;

    k4a::image m_colorXyTable;
    k4a::image m_depthXyTable;
};
} // namespace k4aviewer

#endif
