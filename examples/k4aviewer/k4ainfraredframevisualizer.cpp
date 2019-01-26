/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4ainfraredframevisualizer.h"

// System headers
//

// Library headers
//

// Project headers
//
#include "assertionexception.h"
#include "k4aviewererrormanager.h"

using namespace k4aviewer;

namespace
{
ExpectedValueRange GetExpectedValueRangeForDepthMode(const k4a_depth_mode_t depthMode)
{
    switch (depthMode)
    {
    case K4A_DEPTH_MODE_PASSIVE_IR:
        return { 0, 100 };

    case K4A_DEPTH_MODE_OFF:
        throw AssertionException("Invalid depth mode!");

    default:
        return { 0, 1000 };
    }
}
} // namespace

K4AInfraredFrameVisualizer::K4AInfraredFrameVisualizer(const k4a_depth_mode_t depthMode) :
    K4ADepthSensorFrameBaseVisualizer<K4A_IMAGE_FORMAT_IR16, K4ADepthPixelColorizer::ColorizeGreyscale>(
        depthMode,
        GetExpectedValueRangeForDepthMode(depthMode))
{
}
