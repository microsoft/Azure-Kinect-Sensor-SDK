// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AIMAGEEXTRACTOR_H
#define K4AIMAGEEXTRACTOR_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//

namespace k4aviewer
{

// Lets us use a single function signature to pull different types of image out of a capture
//
class K4AImageExtractor
{
public:
    template<k4a_image_format_t T> static k4a::image GetImageFromCapture(const k4a::capture &capture)
    {
        k4a::image img = capture.get_color_image();
        if (!img || img.get_format() != T)
        {
            return k4a::image();
        }
        return img;
    }
};

template<>
inline k4a::image K4AImageExtractor::GetImageFromCapture<K4A_IMAGE_FORMAT_DEPTH16>(const k4a::capture &capture)
{
    return capture.get_depth_image();
}

template<> inline k4a::image K4AImageExtractor::GetImageFromCapture<K4A_IMAGE_FORMAT_IR16>(const k4a::capture &capture)
{
    return capture.get_ir_image();
}

} // namespace k4aviewer

#endif
