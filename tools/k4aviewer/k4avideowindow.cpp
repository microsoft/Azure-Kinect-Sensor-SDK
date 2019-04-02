// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This causes the explicit template instantiation that we do in this file to not be extern
//
#define K4AVIDEOWINDOW_CPP

// Associated header
//
#include "k4avideowindow.h"

// System headers
//
#include <cmath>

// Library headers
//

// Project headers
//

using namespace k4aviewer;

// Template specialization for RenderInfoPane.  Lets us show pixel value for the depth viewer.
//
template<>
void K4AVideoWindow<K4A_IMAGE_FORMAT_DEPTH16>::RenderInfoPane(const k4a::image &image, const ImVec2 hoveredPixel)
{
    RenderBasicInfoPane(image);
    RenderHoveredDepthPixelValue(image, hoveredPixel, "mm");

    const float sensorTemp = m_imageSource->GetLastSensorTemperature();

    // In recordings, there is no sensor temperature, so it's set to NaN.
    //
    if (!std::isnan(sensorTemp))
    {
        ImGui::Text("Sensor temperature: %.2f C", double(sensorTemp));
    }
}

template<>
void K4AVideoWindow<K4A_IMAGE_FORMAT_IR16>::RenderInfoPane(const k4a::image &image, const ImVec2 hoveredPixel)
{
    RenderBasicInfoPane(image);
    RenderHoveredDepthPixelValue(image, hoveredPixel, "");
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-template-vtables"
#endif

namespace k4aviewer
{
template class K4AVideoWindow<K4A_IMAGE_FORMAT_DEPTH16>;
template class K4AVideoWindow<K4A_IMAGE_FORMAT_IR16>;
} // namespace k4aviewer

#ifdef __clang__
#pragma clang diagnostic pop
#endif
