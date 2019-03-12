// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef GPUDEPTHTOPOINTCLOUDCONVERTER_H
#define GPUDEPTHTOPOINTCLOUDCONVERTER_H

// System headers
//

// Library headers
//
#include <k4a/k4a.hpp>
#include "k4aimgui_all.h"

// Project headers
//
#include "openglhelpers.h"

namespace k4aviewer
{

class GpuDepthToPointCloudConverter
{
public:
    GpuDepthToPointCloudConverter();
    ~GpuDepthToPointCloudConverter() = default;

    // Creates a k4a::image containing the XY tables from calibration based on calibrationType.
    // The table is a 2D array of k4a_float2_t's with the same resolution as the camera of calibrationType
    // specified in calibration.
    //
    // You can use this table to convert a depth image into a point cloud, e.g. by using the Convert method.
    // Conversion is done by multiplying the depth pixel value by the XY table values - i.e. the result
    // pixel will be (xyTable[p].x * depthImage[p], xyTable[p].y * depthImage[p], depthImage[p]), where
    // p is the index of a given pixel.
    //
    static k4a::image GenerateXyTable(const k4a::calibration &calibration, k4a_calibration_type_t calibrationType);

    // Set the XY table that will be used by future calls to Convert().  Get an XY table by calling
    // GenerateXyTable().
    //
    GLenum SetActiveXyTable(const k4a::image &xyTable);

    // Takes depth data and turns it into a texture containing the XYZ coordinates of the depth map
    // using the most recently set-to-active XY table.  The input depth image and output texture
    // (if already set) must be of the same resolution that was used to generate that XY table, or
    // else behavior is undefined.
    //
    // Essentially a reimplementation of k4a::transform::depth_image_to_point_cloud on the GPU.
    // This is much more performant than k4a::transform::depth_image_to_point_cloud, but is a bit
    // more unwieldly to use since you have to use its output in shaders.
    //
    // The output texture has an internal format of GL_RGBA32F and is intended to be used directly
    // by other OpenGL shaders as an image2d uniform.
    //
    // To avoid excess image allocations, you can reuse a texture that was previously output
    // by this function, provided the depth image and XY table previously used was for the same
    // sized texture.
    //
    GLenum Convert(const k4a::image &depth, OpenGL::Texture *texture);

    // The format that the point cloud texture uses internally to store points.
    // If you want to use the texture that this outputs from your shader, you
    // need to pass this as the format argument to glBindImageTexture().
    //
    static constexpr GLenum PointCloudTextureFormat = GL_RGBA32F;

    GpuDepthToPointCloudConverter(GpuDepthToPointCloudConverter &) = delete;
    GpuDepthToPointCloudConverter(GpuDepthToPointCloudConverter &&) = delete;
    GpuDepthToPointCloudConverter &operator=(GpuDepthToPointCloudConverter &) = delete;
    GpuDepthToPointCloudConverter &operator=(GpuDepthToPointCloudConverter &&) = delete;

private:
    OpenGL::Program m_shaderProgram;
    GLint m_destTexId;
    GLint m_xyTableId;
    GLint m_depthImageId;

    OpenGL::Texture m_depthImageTexture;
    OpenGL::Texture m_xyTableTexture;

    OpenGL::Buffer m_depthImagePixelBuffer;
};
} // namespace k4aviewer
#endif
