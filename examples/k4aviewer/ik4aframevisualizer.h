/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef IK4AFRAMEVISUALIZER_H
#define IK4AFRAMEVISUALIZER_H

// System headers
//
#include <memory>

// Library headers
//

// Project headers
//
#include "opengltexture.h"
#include "k4anonbufferingframesource.h"

namespace k4aviewer
{
enum class ImageVisualizationResult
{
    Success,
    OpenGLError,
    InvalidBufferSizeError,
    InvalidImageDataError,
};

inline ImageVisualizationResult GLEnumToImageVisualizationResult(GLenum error)
{
    return error == GL_NO_ERROR ? ImageVisualizationResult::Success : ImageVisualizationResult::OpenGLError;
}

template<k4a_image_format_t ImageFormat> class IK4AFrameVisualizerBase
{
public:
    // Creates a new OpenGL texture from the video source and stores it in texture.
    //
    virtual GLenum InitializeTexture(std::shared_ptr<OpenGlTexture> &texture) = 0;

    // Updates texture in-place with the latest data from the video source.
    // UpdateTexture expects to get a texture that was previously initialized by InitializeTexture.
    //
    virtual ImageVisualizationResult UpdateTexture(std::shared_ptr<OpenGlTexture> &texture,
                                                   const K4AImage<ImageFormat> &frame) = 0;

    virtual ~IK4AFrameVisualizerBase() = default;

    IK4AFrameVisualizerBase() = default;
    IK4AFrameVisualizerBase(const IK4AFrameVisualizerBase &) = delete;
    IK4AFrameVisualizerBase(const IK4AFrameVisualizerBase &&) = delete;
    IK4AFrameVisualizerBase &operator=(const IK4AFrameVisualizerBase &) = delete;
    IK4AFrameVisualizerBase &operator=(const IK4AFrameVisualizerBase &&) = delete;
};

template<k4a_image_format_t ImageFormat> class IK4AFrameVisualizer : public IK4AFrameVisualizerBase<ImageFormat>
{
};

// Depth implementations are expected to also be able to report the value of specific pixels out of a frame
//
template<>
class IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16> : public IK4AFrameVisualizerBase<K4A_IMAGE_FORMAT_DEPTH16>
{
public:
    virtual DepthPixel GetPixel(const K4AImage<K4A_IMAGE_FORMAT_DEPTH16> &frame, size_t x, size_t y) = 0;

    virtual ~IK4AFrameVisualizer() = default;
    IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16>() = default;
    IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16>(const IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16> &) = delete;
    IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16>(const IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16> &&) = delete;
    IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16> &
    operator=(const IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16> &) = delete;
    IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16> &
    operator=(const IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16> &&) = delete;
};
} // namespace k4aviewer
#endif
