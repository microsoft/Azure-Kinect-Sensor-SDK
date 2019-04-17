// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
#include "k4anonbufferingcapturesource.h"
#include "k4apointcloudvisualizer.h"

namespace k4aviewer
{
class K4APointCloudWindow : public IK4AVisualizationWindow
{
public:
    void Show(K4AWindowPlacementInfo placementInfo) override;
    const char *GetTitle() const override;

    K4APointCloudWindow(std::string &&windowTitle,
                        bool enableColorPointCloud,
                        std::shared_ptr<K4ANonBufferingCaptureSource> &&captureSource,
                        const k4a::calibration &calibrationData);
    ~K4APointCloudWindow() override = default;

    K4APointCloudWindow(const K4APointCloudWindow &) = delete;
    K4APointCloudWindow &operator=(const K4APointCloudWindow &) = delete;
    K4APointCloudWindow(const K4APointCloudWindow &&) = delete;
    K4APointCloudWindow &operator=(const K4APointCloudWindow &&) = delete;

private:
    void ProcessInput(ImVec2 imageStartPos, ImVec2 displayDimensions);
    void SetFailed(const char *msg);

    bool CheckVisualizationResult(PointCloudVisualizationResult visualizationResult);

    std::string m_title;
    K4APointCloudVisualizer m_pointCloudVisualizer;
    std::shared_ptr<K4AViewerImage> m_texture;
    std::shared_ptr<K4ANonBufferingCaptureSource> m_captureSource;

    K4APointCloudVisualizer::ColorizationStrategy m_colorizationStrategy =
        K4APointCloudVisualizer::ColorizationStrategy::Shaded;
    int m_pointSize;

    bool m_enableColorPointCloud = false;

    bool m_failed = false;

    static constexpr int MaxConsecutiveMissingImages = 10;
    int m_consecutiveMissingImages = 0;
};
} // namespace k4aviewer

#endif
