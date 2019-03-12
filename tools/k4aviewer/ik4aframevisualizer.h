// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef IK4AFRAMEVISUALIZER_H
#define IK4AFRAMEVISUALIZER_H

// System headers
//
#include <memory>
#include <vector>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//
#include "k4aviewerimage.h"

namespace k4aviewer
{
enum class ImageVisualizationResult
{
    Success,
    OpenGLError,
    InvalidBufferSizeError,
    InvalidImageDataError,
    NoDataError
};

inline ImageVisualizationResult GLEnumToImageVisualizationResult(GLenum error)
{
    return error == GL_NO_ERROR ? ImageVisualizationResult::Success : ImageVisualizationResult::OpenGLError;
}

template<k4a_image_format_t ImageFormat> struct K4ATextureBuffer
{
    std::vector<uint8_t> Data;
    k4a::image SourceImage;
};

template<k4a_image_format_t ImageFormat> class IK4AFrameVisualizer
{
public:
    // Creates a new OpenGL texture from the video source and stores it in texture.
    //
    virtual GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> &texture) = 0;

    // Initializes buffer such that it can hold intermediate images of the type
    // the visualizer handles
    //
    virtual void InitializeBuffer(K4ATextureBuffer<ImageFormat> &buffer) = 0;

    // Interprets image as an image and stores it in buffer in a format that is appropriate for
    // a future call to UpdateTexture
    //
    virtual ImageVisualizationResult ConvertImage(const k4a::image &image, K4ATextureBuffer<ImageFormat> &buffer) = 0;

    // Updates texture in-place with the image stored in buffer.
    // UpdateTexture expects to get a texture that was previously initialized by InitializeTexture.
    //
    virtual ImageVisualizationResult UpdateTexture(const K4ATextureBuffer<ImageFormat> &buffer,
                                                   K4AViewerImage &texture) = 0;

    virtual ~IK4AFrameVisualizer() = default;

    IK4AFrameVisualizer() = default;
    IK4AFrameVisualizer(const IK4AFrameVisualizer &) = delete;
    IK4AFrameVisualizer(const IK4AFrameVisualizer &&) = delete;
    IK4AFrameVisualizer &operator=(const IK4AFrameVisualizer &) = delete;
    IK4AFrameVisualizer &operator=(const IK4AFrameVisualizer &&) = delete;
};
} // namespace k4aviewer

#endif
