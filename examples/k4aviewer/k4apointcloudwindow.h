/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4APOINTCLOUDWINDOW_H
#define K4APOINTCLOUDWINDOW_H

// System headers
//
#include <memory>

// Library headers
//

// Project headers
//
#include "ik4avisualizationwindow.h"
#include "k4acalibrationtransformdata.h"
#include "k4anonbufferingframesource.h"
#include "k4apointcloudvisualizer.h"

namespace k4aviewer
{
class K4APointCloudWindow : public IK4AVisualizationWindow
{
public:
    void Show(K4AWindowPlacementInfo placementInfo) override;
    const char *GetTitle() const override;

    K4APointCloudWindow(std::string &&windowTitle,
                        k4a_depth_mode_t depthMode,
                        std::shared_ptr<K4ANonBufferingFrameSource<K4A_IMAGE_FORMAT_DEPTH16>> &&depthFrameSource,
                        std::unique_ptr<K4ACalibrationTransformData> &&calibrationData);
    ~K4APointCloudWindow() override = default;

    K4APointCloudWindow(const K4APointCloudWindow &) = delete;
    K4APointCloudWindow &operator=(const K4APointCloudWindow &) = delete;
    K4APointCloudWindow(const K4APointCloudWindow &&) = delete;
    K4APointCloudWindow &operator=(const K4APointCloudWindow &&) = delete;

private:
    void ProcessInput();

    std::string m_title;
    K4APointCloudVisualizer m_pointCloudVisualizer;
    std::shared_ptr<OpenGlTexture> m_texture;
    std::shared_ptr<K4ANonBufferingFrameSource<K4A_IMAGE_FORMAT_DEPTH16>> m_depthFrameSource;

    bool m_enableShading = true;

    bool m_failed = false;

    // OpenGL time
    //
    double m_lastTime = 0;
};
} // namespace k4aviewer

#endif
