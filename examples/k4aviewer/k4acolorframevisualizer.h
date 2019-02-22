// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ACOLORFRAMEVISUALIZER_H
#define K4ACOLORFRAMEVISUALIZER_H

// System headers
//
#include <memory>
#include <vector>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "ik4aframevisualizer.h"
#include "k4aviewerutil.h"

namespace k4aviewer
{

class K4AColorFrameVisualizerFactory
{
public:
    template<k4a_image_format_t ImageFormat>
    static std::unique_ptr<IK4AFrameVisualizer<ImageFormat>> Create(k4a_color_resolution_t resolution);
};

template<>
std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_YUY2>>
K4AColorFrameVisualizerFactory::Create<K4A_IMAGE_FORMAT_COLOR_YUY2>(k4a_color_resolution_t resolution);

template<>
std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_NV12>>
K4AColorFrameVisualizerFactory::Create<K4A_IMAGE_FORMAT_COLOR_NV12>(k4a_color_resolution_t resolution);

template<>
std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_BGRA32>>
K4AColorFrameVisualizerFactory::Create<K4A_IMAGE_FORMAT_COLOR_BGRA32>(k4a_color_resolution_t resolution);

template<>
std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_MJPG>>
K4AColorFrameVisualizerFactory::Create<K4A_IMAGE_FORMAT_COLOR_MJPG>(k4a_color_resolution_t resolution);
} // namespace k4aviewer

#endif
