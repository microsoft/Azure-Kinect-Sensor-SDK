// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4awindowset.h"

// System headers
//

// Library headers
//

// Project headers
//
#include "k4aaudiowindow.h"
#include "k4acolorimageconverter.h"
#include "k4adepthimageconverter.h"
#include "k4aimguiextensions.h"
#include "k4aimugraphdatagenerator.h"
#include "k4aimuwindow.h"
#include "k4ainfraredimageconverter.h"
#include "k4apointcloudwindow.h"
#include "k4awindowmanager.h"

using namespace k4aviewer;

namespace
{

template<k4a_image_format_t ImageFormat>
void CreateVideoWindow(const char *sourceIdentifier,
                       const char *windowTitle,
                       K4ADataSource<k4a::capture> &cameraDataSource,
                       std::shared_ptr<K4AConvertingImageSource<ImageFormat>> &&imageSource)
{
    std::string title = std::string(sourceIdentifier) + ": " + windowTitle;

    cameraDataSource.RegisterObserver(imageSource);

    std::unique_ptr<IK4AVisualizationWindow> window(
        std14::make_unique<K4AVideoWindow<ImageFormat>>(std::move(title), imageSource));

    K4AWindowManager::Instance().AddWindow(std::move(window));
}

} // namespace

void K4AWindowSet::ShowModeSelector(ViewType *viewType,
                                    bool enabled,
                                    bool pointCloudViewerEnabled,
                                    const std::function<void(ViewType)> &changeViewFn)
{
    ImGui::Text("View Mode");
    const ViewType oldViewType = *viewType;
    bool modeClicked = false;
    modeClicked |= ImGuiExtensions::K4ARadioButton("2D",
                                                   reinterpret_cast<int *>(viewType),
                                                   static_cast<int>(ViewType::Normal),
                                                   enabled);
    ImGui::SameLine();
    modeClicked |= ImGuiExtensions::K4ARadioButton("3D",
                                                   reinterpret_cast<int *>(viewType),
                                                   static_cast<int>(ViewType::PointCloudViewer),
                                                   pointCloudViewerEnabled && enabled);
    ImGuiExtensions::K4AShowTooltip("Requires depth camera!", !pointCloudViewerEnabled);

    if (modeClicked && oldViewType != *viewType)
    {
        changeViewFn(*viewType);
    }
}

void K4AWindowSet::StartNormalWindows(const char *sourceIdentifier,
                                      K4ADataSource<k4a::capture> *cameraDataSource,
                                      K4ADataSource<k4a_imu_sample_t> *imuDataSource,
                                      std::shared_ptr<K4AMicrophoneListener> &&microphoneDataSource,
                                      bool enableDepthCamera,
                                      k4a_depth_mode_t depthMode,
                                      bool enableColorCamera,
                                      k4a_image_format_t colorFormat,
                                      k4a_color_resolution_t colorResolution)
{
    if (cameraDataSource != nullptr)
    {
        if (enableDepthCamera)
        {
            CreateVideoWindow(sourceIdentifier,
                              "Infrared Camera",
                              *cameraDataSource,
                              std::make_shared<K4AConvertingImageSource<K4A_IMAGE_FORMAT_IR16>>(
                                  std14::make_unique<K4AInfraredImageConverter>(depthMode)));

            // K4A_DEPTH_MODE_PASSIVE_IR doesn't support actual depth
            //
            if (depthMode != K4A_DEPTH_MODE_PASSIVE_IR)
            {
                CreateVideoWindow(sourceIdentifier,
                                  "Depth Camera",
                                  *cameraDataSource,
                                  std::make_shared<K4AConvertingImageSource<K4A_IMAGE_FORMAT_DEPTH16>>(
                                      std14::make_unique<K4ADepthImageConverter>(depthMode)));
            }
        }

        if (enableColorCamera)
        {
            constexpr char colorWindowTitle[] = "Color Camera";

            switch (colorFormat)
            {
            case K4A_IMAGE_FORMAT_COLOR_YUY2:
                CreateVideoWindow<K4A_IMAGE_FORMAT_COLOR_YUY2>(
                    sourceIdentifier,
                    colorWindowTitle,
                    *cameraDataSource,
                    std::make_shared<K4AConvertingImageSource<K4A_IMAGE_FORMAT_COLOR_YUY2>>(
                        K4AColorImageConverterFactory::Create<K4A_IMAGE_FORMAT_COLOR_YUY2>(colorResolution)));
                break;

            case K4A_IMAGE_FORMAT_COLOR_MJPG:
                CreateVideoWindow<K4A_IMAGE_FORMAT_COLOR_MJPG>(
                    sourceIdentifier,
                    colorWindowTitle,
                    *cameraDataSource,
                    std::make_shared<K4AConvertingImageSource<K4A_IMAGE_FORMAT_COLOR_MJPG>>(
                        K4AColorImageConverterFactory::Create<K4A_IMAGE_FORMAT_COLOR_MJPG>(colorResolution)));
                break;

            case K4A_IMAGE_FORMAT_COLOR_BGRA32:
                CreateVideoWindow<K4A_IMAGE_FORMAT_COLOR_BGRA32>(
                    sourceIdentifier,
                    colorWindowTitle,
                    *cameraDataSource,
                    std::make_shared<K4AConvertingImageSource<K4A_IMAGE_FORMAT_COLOR_BGRA32>>(
                        K4AColorImageConverterFactory::Create<K4A_IMAGE_FORMAT_COLOR_BGRA32>(colorResolution)));
                break;

            case K4A_IMAGE_FORMAT_COLOR_NV12:
                CreateVideoWindow<K4A_IMAGE_FORMAT_COLOR_NV12>(
                    sourceIdentifier,
                    colorWindowTitle,
                    *cameraDataSource,
                    std::make_shared<K4AConvertingImageSource<K4A_IMAGE_FORMAT_COLOR_NV12>>(
                        K4AColorImageConverterFactory::Create<K4A_IMAGE_FORMAT_COLOR_NV12>(colorResolution)));
                break;

            default:
                K4AViewerErrorManager::Instance().SetErrorStatus("Invalid color mode!");
            }
        }
    }

    // Build a collection of the graph-type windows we're using so the window manager knows it can group them
    // in the same section
    //
    std::vector<std::unique_ptr<IK4AVisualizationWindow>> graphWindows;
    if (imuDataSource != nullptr)
    {
        std::string title = std::string(sourceIdentifier) + ": IMU Data";

        auto imuGraphDataGenerator = std::make_shared<K4AImuGraphDataGenerator>();
        imuDataSource->RegisterObserver(std::static_pointer_cast<IK4AImuObserver>(imuGraphDataGenerator));

        graphWindows.emplace_back(std14::make_unique<K4AImuWindow>(std::move(title), std::move(imuGraphDataGenerator)));
    }

    if (microphoneDataSource != nullptr)
    {
        std::string micTitle = std::string(sourceIdentifier) + ": Microphone Data";

        graphWindows.emplace_back(std::unique_ptr<IK4AVisualizationWindow>(
            std14::make_unique<K4AAudioWindow>(std::move(micTitle), std::move(microphoneDataSource))));
    }

    if (!graphWindows.empty())
    {
        K4AWindowManager::Instance().AddWindowGroup(std::move(graphWindows));
    }
}

void K4AWindowSet::StartPointCloudWindow(const char *sourceIdentifier,
                                         const k4a::calibration &calibrationData,
                                         K4ADataSource<k4a::capture> *cameraDataSource,
                                         bool enableColorPointCloud)
{
    std::string pointCloudTitle = std::string(sourceIdentifier) + ": Point Cloud Viewer";
    auto captureSource = std::make_shared<K4ANonBufferingCaptureSource>();
    cameraDataSource->RegisterObserver(captureSource);

    auto &wm = K4AWindowManager::Instance();
    wm.AddWindow(std14::make_unique<K4APointCloudWindow>(std::move(pointCloudTitle),
                                                         enableColorPointCloud,
                                                         std::move(captureSource),
                                                         calibrationData));
}
