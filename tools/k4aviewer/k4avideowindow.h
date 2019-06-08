// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AVIDEOWINDOW_H
#define K4AVIDEOWINDOW_H

// System headers
//
#include <memory>
#include <sstream>
#include <string>

// Library headers
//

// Project headers
//
#include "ik4avisualizationwindow.h"
#include "k4aconvertingimagesource.h"
#include "k4aviewererrormanager.h"
#include "k4aviewersettingsmanager.h"
#include "k4awindowsizehelpers.h"
#include "k4aviewerimage.h"

namespace k4aviewer
{
template<k4a_image_format_t ImageFormat> class K4AVideoWindow : public IK4AVisualizationWindow
{
public:
    K4AVideoWindow(std::string &&title, std::shared_ptr<K4AConvertingImageSource<ImageFormat>> imageSource) :
        m_imageSource(std::move(imageSource)),
        m_title(std::move(title))
    {
        const GLenum initResult = m_imageSource->InitializeTexture(&m_currentTexture);
        const ImageConversionResult ivr = GLEnumToImageConversionResult(initResult);
        if (ivr != ImageConversionResult::Success)
        {
            SetFailed(ivr);
        }
    }

    ~K4AVideoWindow() override = default;

    void Show(K4AWindowPlacementInfo placementInfo) override
    {
        if (m_failed)
        {
            ImGui::Text("Video playback failed!");
            return;
        }

        if (m_failed)
        {
            return;
        }

        const ImageConversionResult nextImageResult = m_imageSource->GetNextImage(m_currentTexture.get(),
                                                                                  &m_currentImage);
        if (nextImageResult == ImageConversionResult::NoDataError)
        {
            // We don't have data from the camera yet; show the window with the default black texture.
            // Continue rendering the window.
        }
        else if (nextImageResult != ImageConversionResult::Success)
        {
            SetFailed(nextImageResult);
            return;
        }

        // The absolute coordinates where the next widget will be drawn.  This call must
        // be before any widgets are drawn on the window or else our cursor math will
        // think the root of the window is after those widgets.  Used for calculating the
        // hovered pixel and where to put the overlay.
        //
        const ImVec2 imageStartPos = ImGui::GetCursorScreenPos();

        const ImVec2 sourceImageDimensions = { static_cast<float>(m_currentTexture->GetDimensions().Width),
                                               static_cast<float>(m_currentTexture->GetDimensions().Height) };

        // Compute how big we can make the image
        //
        const ImVec2 displayDimensions = GetMaxImageSize(sourceImageDimensions, placementInfo.Size);

        ImGui::Image(static_cast<ImTextureID>(*m_currentTexture), displayDimensions);

        const bool imageIsHovered = ImGui::IsItemHovered();

        if (m_currentImage != nullptr &&
            K4AViewerSettingsManager::Instance().GetViewerOption(ViewerOption::ShowInfoPane))
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(0.f, 0.f), displayDimensions);
            ImGui::SetNextWindowPos(imageStartPos, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background
            ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar;

            std::string overlayTitle = m_title + "##overlay";
            if (ImGui::Begin(overlayTitle.c_str(), nullptr, overlayFlags))
            {
                ImVec2 hoveredImagePixel = InvalidHoveredPixel;

                // The overlay obstructs hover detection on the image, so we have to check if the overlay is hovered too
                //
                if (imageIsHovered || ImGui::IsWindowHovered())
                {
                    // Compute hovered pixel coordinates
                    //
                    const ImVec2 mousePos = ImGui::GetIO().MousePos;
                    ImVec2 hoveredUIPixel;
                    hoveredUIPixel.x = mousePos.x - imageStartPos.x;
                    hoveredUIPixel.x = std::min(hoveredUIPixel.x, displayDimensions.x);
                    hoveredUIPixel.x = std::max(hoveredUIPixel.x, 0.0f);

                    hoveredUIPixel.y = mousePos.y - imageStartPos.y;
                    hoveredUIPixel.y = std::min(hoveredUIPixel.y, displayDimensions.y);
                    hoveredUIPixel.y = std::max(hoveredUIPixel.y, 0.0f);

                    const float uiCoordinateToImageCoordinateRatio = sourceImageDimensions.x / displayDimensions.x;

                    hoveredImagePixel.x = hoveredUIPixel.x * uiCoordinateToImageCoordinateRatio;
                    hoveredImagePixel.y = hoveredUIPixel.y * uiCoordinateToImageCoordinateRatio;
                }

                RenderInfoPane(m_currentImage, hoveredImagePixel);
            }
            ImGui::End();
        }
    }

    const char *GetTitle() const override
    {
        return m_title.c_str();
    }

    K4AVideoWindow(const K4AVideoWindow &) = delete;
    K4AVideoWindow &operator=(const K4AVideoWindow &) = delete;
    K4AVideoWindow(const K4AVideoWindow &&) = delete;
    K4AVideoWindow &operator=(const K4AVideoWindow &&) = delete;

private:
    const ImVec2 InvalidHoveredPixel = ImVec2(-1, -1);

    void RenderInfoPane(const k4a::image &image, ImVec2 hoveredPixel)
    {
        (void)hoveredPixel;
        RenderBasicInfoPane(image);
    }

    void RenderBasicInfoPane(const k4a::image &image)
    {
        if (K4AViewerSettingsManager::Instance().GetViewerOption(ViewerOption::ShowFrameRateInfo))
        {
            ImGui::Text("Average frame rate: %.2f fps", m_imageSource->GetFrameRate());
        }

        ImGui::Text("Timestamp: %llu", static_cast<long long int>(image.get_device_timestamp().count()));
    }

    void RenderHoveredDepthPixelValue(const k4a::image &depthImage, const ImVec2 hoveredPixel, const char *units)
    {
        if (static_cast<int>(hoveredPixel.x) == static_cast<int>(InvalidHoveredPixel.x) &&
            static_cast<int>(hoveredPixel.y) == static_cast<int>(InvalidHoveredPixel.y))
        {
            return;
        }

        DepthPixel pixelValue = 0;
        if (hoveredPixel.x >= 0 && hoveredPixel.y >= 0)
        {
            const DepthPixel *buffer = reinterpret_cast<const DepthPixel *>(depthImage.get_buffer());
            pixelValue =
                buffer[size_t(hoveredPixel.y) * size_t(depthImage.get_width_pixels()) + size_t(hoveredPixel.x)];
        }

        ImGui::Text("Current pixel: %d, %d", int(hoveredPixel.x), int(hoveredPixel.y));
        ImGui::Text("Current pixel value: %d %s", pixelValue, units);
    }

    // Sets the window failed with an error message derived from errorCode
    //
    void SetFailed(const ImageConversionResult errorCode)
    {
        std::stringstream errorBuilder;
        errorBuilder << m_title << ": ";
        switch (errorCode)
        {
        case ImageConversionResult::InvalidBufferSizeError:
            errorBuilder << "received an unexpected amount of data!";
            break;

        case ImageConversionResult::InvalidImageDataError:
            errorBuilder << "received malformed image data!";
            break;

        case ImageConversionResult::OpenGLError:
            errorBuilder << "failed to upload image to OpenGL!";
            break;

        default:
            errorBuilder << "unknown error!";
            break;
        }

        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());

        m_failed = true;
    }

    std::shared_ptr<K4AConvertingImageSource<ImageFormat>> m_imageSource;
    std::string m_title;
    bool m_failed = false;

    k4a::image m_currentImage;
    std::shared_ptr<K4AViewerImage> m_currentTexture;
};

template<> void K4AVideoWindow<K4A_IMAGE_FORMAT_DEPTH16>::RenderInfoPane(const k4a::image &, ImVec2 hoveredPixel);
template<> void K4AVideoWindow<K4A_IMAGE_FORMAT_IR16>::RenderInfoPane(const k4a::image &, ImVec2 hoveredPixel);

// Template specialization for RenderInfoPane.  Lets us show pixel value for depth and IR sensors and temperature
// data for the depth sensor.
//
#ifndef K4AVIDEOWINDOW_CPP
extern template class K4AVideoWindow<K4A_IMAGE_FORMAT_DEPTH16>;
extern template class K4AVideoWindow<K4A_IMAGE_FORMAT_IR16>;
#endif
} // namespace k4aviewer

#endif
