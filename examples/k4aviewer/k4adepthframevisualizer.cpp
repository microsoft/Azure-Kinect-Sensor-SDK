/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4adepthframevisualizer.h"

// System headers
//

// Library headers
//

// Project headers
//
#include "assertionexception.h"
#include "k4adepthpixelcolorizer.h"
#include "k4aviewererrormanager.h"
#include "k4aviewerutil.h"

using namespace k4aviewer;

K4ADepthFrameVisualizer::K4ADepthFrameVisualizer(const k4a_depth_mode_t depthMode) :
    K4ADepthSensorFrameBaseVisualizer(depthMode, GetRangeForDepthMode(depthMode))
{
}

DepthPixel K4ADepthFrameVisualizer::GetPixel(const K4AImage<K4A_IMAGE_FORMAT_DEPTH16> &frame,
                                             const size_t x,
                                             const size_t y)
{
    const size_t pixelOffset = y * static_cast<size_t>(GetDimensions().Width) + x;
    if (pixelOffset * sizeof(DepthPixel) > frame.GetSize())
    {
        throw AssertionException("Invalid coordinates for frame!");
    }

    const DepthPixel pixelValue = reinterpret_cast<DepthPixel *>(frame.GetBuffer())[pixelOffset];
    return pixelValue;
}
