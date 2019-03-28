// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef IK4AIMAGECONVERTER_H
#define IK4AIMAGECONVERTER_H

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
enum class ImageConversionResult
{
    Success,
    OpenGLError,
    InvalidBufferSizeError,
    InvalidImageDataError,
    NoDataError
};

inline ImageConversionResult GLEnumToImageConversionResult(GLenum error)
{
    return error == GL_NO_ERROR ? ImageConversionResult::Success : ImageConversionResult::OpenGLError;
}

template<k4a_image_format_t ImageFormat> class IK4AImageConverter
{
public:
    // Gets the dimensions in pixels of images that this converter expects to receive and produce
    //
    virtual ImageDimensions GetImageDimensions() const = 0;

    // Converts srcImage into a BGRA32-formatted image and stores the result in bgraImage.
    // bgraImage must already be allocated and must be an image with the same dimensions that
    // GetImageDimensions() returns.
    //
    virtual ImageConversionResult ConvertImage(const k4a::image &srcImage, k4a::image *bgraImage) = 0;

    virtual ~IK4AImageConverter() = default;

    IK4AImageConverter() = default;
    IK4AImageConverter(const IK4AImageConverter &) = delete;
    IK4AImageConverter(const IK4AImageConverter &&) = delete;
    IK4AImageConverter &operator=(const IK4AImageConverter &) = delete;
    IK4AImageConverter &operator=(const IK4AImageConverter &&) = delete;
};
} // namespace k4aviewer

#endif
