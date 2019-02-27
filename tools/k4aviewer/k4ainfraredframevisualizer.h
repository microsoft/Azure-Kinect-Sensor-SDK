// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AINFRAREDFRAMEVISUALIZER_H
#define K4AINFRAREDFRAMEVISUALIZER_H

// System headers
//

// Library headers
//

// Project headers
//
#include "k4adepthsensorframebasevisualizer.h"
#include "k4adepthpixelcolorizer.h"

namespace k4aviewer
{
class K4AInfraredFrameVisualizer
    : public K4ADepthSensorFrameBaseVisualizer<K4A_IMAGE_FORMAT_IR16, K4ADepthPixelColorizer::ColorizeGreyscale>
{
public:
    explicit K4AInfraredFrameVisualizer(k4a_depth_mode_t depthMode);

    ~K4AInfraredFrameVisualizer() override = default;

    K4AInfraredFrameVisualizer(const K4AInfraredFrameVisualizer &) = delete;
    K4AInfraredFrameVisualizer(const K4AInfraredFrameVisualizer &&) = delete;
    K4AInfraredFrameVisualizer &operator=(const K4AInfraredFrameVisualizer &) = delete;
    K4AInfraredFrameVisualizer &operator=(const K4AInfraredFrameVisualizer &&) = delete;
};
} // namespace k4aviewer

#endif
