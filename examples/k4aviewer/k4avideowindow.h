/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4AVIDEOWINDOW_H
#define K4AVIDEOWINDOW_H

// System headers
//
#include <memory>
#include <sstream>
#include <string>

// Library headers
//
#include "opengltexture.h"

// Project headers
//
#include "ik4aframevisualizer.h"
#include "ik4avisualizationwindow.h"
#include "k4aviewererrormanager.h"
#include "k4aviewersettingsmanager.h"
#include "k4awindowsizehelpers.h"

namespace k4aviewer
{
template<k4a_image_format_t ImageFormat> class K4AVideoWindow : public IK4AVisualizationWindow
{
public:
    K4AVideoWindow(std::string &&title,
                   std::unique_ptr<IK4AFrameVisualizer<ImageFormat>> &&frameVisualizer,
                   std::shared_ptr<K4ANonBufferingFrameSource<ImageFormat>> frameSource) :
        m_frameVisualizer(std::move(frameVisualizer)),
        m_frameSource(std::move(frameSource)),
        m_title(std::move(title))
    {
        const GLenum initResult = m_frameVisualizer->InitializeTexture(m_currentTexture);
        CheckImageVisualizationResult(GLEnumToImageVisualizationResult(initResult));
    }

    ~K4AVideoWindow() override = default;

    void Show(K4AWindowPlacementInfo placementInfo) override
    {
        if (m_failed)
        {
            ImGui::Text("Video playback failed!");
            return;
        }

        RenderVideoFrame(placementInfo.Size);
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
    void RenderVideoFrame(ImVec2 maxSize)
    {
        if (m_frameSource->IsFailed())
        {
            K4AViewerErrorManager::Instance().SetErrorStatus(m_title + ": frame source failed!");

            m_failed = true;
            return;
        }

        // If we haven't received data from the camera yet, we just show the default texture (all black).
        //
        std::shared_ptr<K4AImage<ImageFormat>> frame;
        if (m_frameSource->HasData())
        {
            frame = m_frameSource->GetLastFrame();

            // Turn camera data into an OpenGL texture so we can give it to ImGui
            //
            if (!CheckImageVisualizationResult(m_frameVisualizer->UpdateTexture(m_currentTexture, *frame)))
            {
                return;
            }
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
        const ImVec2 displayDimensions = GetImageSize(sourceImageDimensions, maxSize);

        ImGui::Image(static_cast<ImTextureID>(*m_currentTexture), displayDimensions);

        const bool imageIsHovered = ImGui::IsItemHovered();

        if (frame && K4AViewerSettingsManager::Instance().GetShowInfoPane())
        {
            ImGui::SetNextWindowPos(imageStartPos, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background
            ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoNav;

            std::string overlayTitle = m_title + "##overlay";
            if (ImGui::Begin(overlayTitle.c_str(), nullptr, overlayFlags))
            {
                ImVec2 hoveredImagePixel(-1, -1);

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

                RenderInfoPane(*frame, hoveredImagePixel);
            }
            ImGui::End();
        }
    }

    void RenderInfoPane(const K4AImage<ImageFormat> &frame, ImVec2 hoveredPixel)
    {
        (void)hoveredPixel;
        RenderBasicInfoPane(frame);
    }

    void RenderBasicInfoPane(const K4AImage<ImageFormat> &frame)
    {
        if (K4AViewerSettingsManager::Instance().GetShowFrameRateInfo())
        {
            ImGui::Text("Average frame rate: %.2f fps", m_frameSource->GetFrameRate());
        }

        ImGui::Text("Timestamp: %llu", static_cast<long long unsigned int>(frame.GetTimestampUsec()));
    }

    // If errorCode is successful, returns true; otherwise, raises an error, marks the window
    // as failed, and raises an error message to the user.
    //
    bool CheckImageVisualizationResult(const ImageVisualizationResult errorCode)
    {
        if (errorCode == ImageVisualizationResult::Success)
        {
            return true;
        }

        std::stringstream errorBuilder;
        errorBuilder << m_title << ": ";
        switch (errorCode)
        {
        case ImageVisualizationResult::InvalidBufferSizeError:
            errorBuilder << "received an unexpected amount of data!";
            break;

        case ImageVisualizationResult::InvalidImageDataError:
            errorBuilder << "received malformed image data!";
            break;

        case ImageVisualizationResult::OpenGLError:
            errorBuilder << "failed to upload image to OpenGL!";
            break;

        default:
            errorBuilder << "unknown error!";
            break;
        }

        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());

        m_failed = true;
        return false;
    }

    std::unique_ptr<IK4AFrameVisualizer<ImageFormat>> m_frameVisualizer;
    std::shared_ptr<K4ANonBufferingFrameSource<ImageFormat>> m_frameSource;
    std::string m_title;
    bool m_failed = false;

    std::shared_ptr<OpenGlTexture> m_currentTexture;
};

template<>
void K4AVideoWindow<K4A_IMAGE_FORMAT_DEPTH16>::RenderInfoPane(const K4AImage<K4A_IMAGE_FORMAT_DEPTH16> &,
                                                              ImVec2 hoveredPixel);

// Template specialization for RenderInfoPane.  Lets us show pixel value for the depth viewer.
//
#ifndef K4AVIDEOWINDOW_CPP
extern template class K4AVideoWindow<K4A_IMAGE_FORMAT_DEPTH16>;
#endif
} // namespace k4aviewer

#endif
