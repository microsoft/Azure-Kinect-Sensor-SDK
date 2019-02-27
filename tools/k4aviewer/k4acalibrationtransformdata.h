// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ACALIBRATIONTRANSFORMDATA_H
#define K4ACALIBRATIONTRANSFORMDATA_H

// System headers
//
#include <vector>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include "k4arecording.h"

namespace k4aviewer
{
class K4ACalibrationTransformData
{
public:
    K4ACalibrationTransformData() = default;

    ~K4ACalibrationTransformData();

    k4a_calibration_t CalibrationData{};

    int DepthWidth = 0;
    int DepthHeight = 0;

    k4a_image_t PointCloudImage = nullptr;

    k4a_transformation_t TransformationHandle = nullptr;

private:
    k4a_result_t Initialize(const k4a_device_t &device, k4a_depth_mode_t depthMode, k4a_color_resolution_t resolution);

    k4a_result_t Initialize(const k4a_playback_t &playback);

    k4a_result_t CommonInitialize();

    friend class K4ADevice;
    friend class K4ARecording;
};
} // namespace k4aviewer

#endif
