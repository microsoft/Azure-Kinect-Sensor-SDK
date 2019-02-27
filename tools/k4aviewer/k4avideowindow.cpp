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
void K4AVideoWindow<K4A_IMAGE_FORMAT_DEPTH16>::RenderInfoPane(const K4AImage<K4A_IMAGE_FORMAT_DEPTH16> &frame,
                                                              const ImVec2 hoveredPixel)
{
    RenderBasicInfoPane(frame);

    DepthPixel pixelValue = 0;
    if (hoveredPixel.x >= 0 && hoveredPixel.y >= 0)
    {
        const DepthPixel *buffer = reinterpret_cast<DepthPixel *>(m_currentImage->GetBuffer());
        pixelValue = buffer[size_t(hoveredPixel.y) * size_t(m_currentImage->GetWidthPixels()) + size_t(hoveredPixel.x)];
    }

    ImGui::Text("Current pixel: %d, %d", int(hoveredPixel.x), int(hoveredPixel.y));
    ImGui::Text("Current pixel value: %d mm", pixelValue);

    // In recordings, there is no sensor temperature, so it's set to NaN.
    //
    if (!std::isnan(m_frameSource->GetLastSensorTemperature()))
    {
        ImGui::Text("Sensor temperature: %.2f C", double(m_frameSource->GetLastSensorTemperature()));
    }
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-template-vtables"
#endif

namespace k4aviewer
{
template class K4AVideoWindow<K4A_IMAGE_FORMAT_DEPTH16>;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
