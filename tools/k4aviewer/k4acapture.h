// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ACAPTURE_H
#define K4ACAPTURE_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include "k4aimage.h"

namespace k4aviewer
{

class K4ACapture
{
public:
    explicit K4ACapture(k4a_capture_t capture);
    ~K4ACapture();

    template<k4a_image_format_t T> std::shared_ptr<K4AImage<T>> GetColorImage()
    {
        return K4AImageFactory::CreateK4AImage<T>(k4a_capture_get_color_image(m_capture));
    }
    std::shared_ptr<K4AImage<K4A_IMAGE_FORMAT_DEPTH16>> GetDepthImage();
    std::shared_ptr<K4AImage<K4A_IMAGE_FORMAT_IR16>> GetIrImage();

    float GetTemperature() const;

    K4ACapture(const K4ACapture &) = delete;
    K4ACapture(const K4ACapture &&) = delete;
    K4ACapture &operator=(const K4ACapture &) = delete;
    K4ACapture &operator=(const K4ACapture &&) = delete;

private:
    k4a_capture_t m_capture;
};
} // namespace k4aviewer

#endif
