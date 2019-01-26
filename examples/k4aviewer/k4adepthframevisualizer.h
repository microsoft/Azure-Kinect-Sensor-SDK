/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ADEPTHFRAMEVISUALIZER_H
#define K4ADEPTHFRAMEVISUALIZER_H

// System headers
//

// Library headers
//

// Project headers
//
#include "k4adepthsensorframebasevisualizer.h"
#include "k4anonbufferingframesource.h"
#include "k4adepthpixelcolorizer.h"

namespace k4aviewer
{

class K4ADepthFrameVisualizer
    : public K4ADepthSensorFrameBaseVisualizer<K4A_IMAGE_FORMAT_DEPTH16, K4ADepthPixelColorizer::ColorizeRedToBlue>
{
public:
    explicit K4ADepthFrameVisualizer(k4a_depth_mode_t depthMode);

    DepthPixel GetPixel(const K4AImage<K4A_IMAGE_FORMAT_DEPTH16> &frame, size_t x, size_t y) override;

    ~K4ADepthFrameVisualizer() override = default;

    K4ADepthFrameVisualizer(const K4ADepthFrameVisualizer &) = delete;
    K4ADepthFrameVisualizer(const K4ADepthFrameVisualizer &&) = delete;
    K4ADepthFrameVisualizer &operator=(const K4ADepthFrameVisualizer &) = delete;
    K4ADepthFrameVisualizer &operator=(const K4ADepthFrameVisualizer &&) = delete;
};
} // namespace k4aviewer

#endif
