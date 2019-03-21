// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ADEPTHFRAMEVISUALIZER_H
#define K4ADEPTHFRAMEVISUALIZER_H

// System headers
//

// Library headers
//

// Project headers
//
#include "k4adepthsensorframebasevisualizer.h"
#include "k4adepthpixelcolorizer.h"
#include "k4astaticimageproperties.h"

namespace k4aviewer
{

class K4ADepthFrameVisualizer
    : public K4ADepthSensorFrameBaseVisualizer<K4A_IMAGE_FORMAT_DEPTH16, K4ADepthPixelColorizer::ColorizeBlueToRed>
{
public:
    explicit K4ADepthFrameVisualizer(k4a_depth_mode_t depthMode) :
        K4ADepthSensorFrameBaseVisualizer(depthMode, GetDepthModeRange(depthMode))
    {
    }

    ~K4ADepthFrameVisualizer() override = default;

    K4ADepthFrameVisualizer(const K4ADepthFrameVisualizer &) = delete;
    K4ADepthFrameVisualizer(const K4ADepthFrameVisualizer &&) = delete;
    K4ADepthFrameVisualizer &operator=(const K4ADepthFrameVisualizer &) = delete;
    K4ADepthFrameVisualizer &operator=(const K4ADepthFrameVisualizer &&) = delete;
};
} // namespace k4aviewer

#endif
