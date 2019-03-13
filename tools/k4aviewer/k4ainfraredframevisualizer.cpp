// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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

K4AInfraredFrameVisualizer::K4AInfraredFrameVisualizer(const k4a_depth_mode_t depthMode) :
    K4ADepthSensorFrameBaseVisualizer<K4A_IMAGE_FORMAT_IR16, K4ADepthPixelColorizer::ColorizeGreyscale>(depthMode,
                                                                                                        GetIrRange(
                                                                                                            depthMode))
{
}
