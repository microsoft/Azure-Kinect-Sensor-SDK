/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4AWINDOWSET_H
#define K4AWINDOWSET_H

// Associated header
//

// System headers
//
#include <functional>
#include <memory>

// Library headers
//
#include "k4aimgui_all.h"
#include <k4a/k4a.h>

// Project headers
//
#include "k4acalibrationtransformdata.h"
#include "k4adatasource.h"
#include "k4amicrophonelistener.h"
#include "k4avideowindow.h"

namespace k4aviewer
{
class K4AWindowSet
{
public:
    enum class ViewType
    {
        Normal,
        PointCloudViewer
    };
    static void ShowModeSelector(ViewType *viewType,
                                 bool enabled,
                                 bool pointCloudViewerEnabled,
                                 const std::function<void(ViewType)> &changeViewFn);

    static void StartNormalWindows(const char *sourceIdentifier,
                                   K4ADataSource<std::shared_ptr<K4ACapture>> *cameraDataSource,
                                   K4ADataSource<k4a_imu_sample_t> *imuDataSource,
                                   std::shared_ptr<K4AMicrophoneListener> &&microphoneDataSource,
                                   bool enableDepthCamera,
                                   k4a_depth_mode_t depthMode,
                                   bool enableColorCamera,
                                   k4a_image_format_t colorFormat,
                                   k4a_color_resolution_t colorResolution);

    static void StartPointCloudWindow(const char *sourceIdentifier,
                                      std::unique_ptr<K4ACalibrationTransformData> &&calibrationData,
                                      K4ADataSource<std::shared_ptr<K4ACapture>> &cameraDataSource,
                                      k4a_depth_mode_t depthMode);
};
} // namespace k4aviewer

#endif
