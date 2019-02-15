/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4AIMAGEEXTRACTOR_H
#define K4AIMAGEEXTRACTOR_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include "k4acapture.h"

namespace k4aviewer
{

// Lets us use a single function signature to pull different types of image out of a capture
//
class K4AImageExtractor
{
public:
    template<k4a_image_format_t T>
    static std::shared_ptr<K4AImage<T>> GetImageFromCapture(const std::shared_ptr<K4ACapture> &capture)
    {
        return capture->GetColorImage<T>();
    }
};

template<>
inline std::shared_ptr<K4AImage<K4A_IMAGE_FORMAT_DEPTH16>>
K4AImageExtractor::GetImageFromCapture<K4A_IMAGE_FORMAT_DEPTH16>(const std::shared_ptr<K4ACapture> &capture)
{
    return capture->GetDepthImage();
}

template<>
inline std::shared_ptr<K4AImage<K4A_IMAGE_FORMAT_IR16>>
K4AImageExtractor::GetImageFromCapture<K4A_IMAGE_FORMAT_IR16>(const std::shared_ptr<K4ACapture> &capture)
{
    return capture->GetIrImage();
}

} // namespace k4aviewer

#endif
