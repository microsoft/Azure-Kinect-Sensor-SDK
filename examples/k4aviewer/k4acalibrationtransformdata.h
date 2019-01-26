/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

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

namespace k4aviewer
{
class K4ACalibrationTransformData
{
public:
    K4ACalibrationTransformData();

    ~K4ACalibrationTransformData();

    k4a_calibration_t CalibrationData{};

    int DepthWidth{};
    int DepthHeight{};
    k4a_image_t PointCloudImage{};

    k4a_transformation_t TransformationHandle{};

private:
    k4a_result_t Initialize(const k4a_device_t &device, k4a_depth_mode_t depthMode, k4a_color_resolution_t resolution);

    friend class K4ADevice;
};
} // namespace k4aviewer

#endif
