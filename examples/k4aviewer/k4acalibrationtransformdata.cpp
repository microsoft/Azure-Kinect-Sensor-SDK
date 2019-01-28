/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4acalibrationtransformdata.h"

// System headers
//

// Library headers
//

// Project headers
//

using namespace k4aviewer;

K4ACalibrationTransformData::~K4ACalibrationTransformData()
{
    if (TransformationHandle != nullptr)
    {
        k4a_transformation_destroy(TransformationHandle);
    }
    if (PointCloudImage != nullptr)
    {
        k4a_image_release(PointCloudImage);
    }
}

k4a_result_t K4ACalibrationTransformData::Initialize(const k4a_device_t &device,
                                                     const k4a_depth_mode_t depthMode,
                                                     const k4a_color_resolution_t resolution)
{
    const k4a_result_t result = k4a_device_get_calibration(device, depthMode, resolution, &CalibrationData);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        return result;
    }

    return CommonInitialize();
}

k4a_result_t K4ACalibrationTransformData::Initialize(const k4a_playback_t &playback)
{
    const k4a_result_t result = k4a_playback_get_calibration(playback, &CalibrationData);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        return result;
    }

    return CommonInitialize();
}

k4a_result_t K4ACalibrationTransformData::CommonInitialize()
{
    DepthWidth = CalibrationData.depth_camera_calibration.resolution_width;
    DepthHeight = CalibrationData.depth_camera_calibration.resolution_height;

    const k4a_result_t result = k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
                                                 DepthWidth,
                                                 DepthHeight,
                                                 DepthWidth * 3 * static_cast<int>(sizeof(int16_t)),
                                                 &PointCloudImage);

    if (result != K4A_RESULT_SUCCEEDED)
    {
        return result;
    }

    TransformationHandle = k4a_transformation_create(&CalibrationData);

    return K4A_RESULT_SUCCEEDED;
}
