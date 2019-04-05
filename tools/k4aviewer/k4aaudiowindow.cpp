// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aaudiowindow.h"

// System headers
//
#include <sstream>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4aimguiextensions.h"
#include "k4amicrophonelistener.h"
#include "k4aviewererrormanager.h"
#include "k4awindowsizehelpers.h"

using namespace k4aviewer;

namespace
{
constexpr float MinHeight = 140.f;
}

K4AAudioWindow::K4AAudioWindow(std::string &&title, std::shared_ptr<K4AMicrophoneListener> listener) :
    m_title(std::move(title)),
    m_listener(std::move(listener)),
    m_channelData{ {
        K4AAudioChannelDataGraph("Channel 0"),
        K4AAudioChannelDataGraph("Channel 1"),
        K4AAudioChannelDataGraph("Channel 2"),
        K4AAudioChannelDataGraph("Channel 3"),
        K4AAudioChannelDataGraph("Channel 4"),
        K4AAudioChannelDataGraph("Channel 5"),
        K4AAudioChannelDataGraph("Channel 6"),
    } }
{
}

void K4AAudioWindow::Show(K4AWindowPlacementInfo placementInfo)
{
    ProcessNewData();

    if (m_listener && m_listener->GetStatus() != SoundIoErrorNone)
    {
        std::stringstream errorBuilder;
        errorBuilder << "Microphone failed: " << soundio_strerror(m_listener->GetStatus()) << "!";
        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
        m_listener.reset();
    }

    if (!m_listener)
    {
        ImGui::Text("Microphone failed!");
        return;
    }

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec2 sliderSize;

    sliderSize.x = GetStandardVerticalSliderWidth();
    sliderSize.y = placementInfo.Size.y;
    sliderSize.y -= GetTitleBarHeight();
    sliderSize.y -= 2 * style.WindowPadding.y;
    sliderSize.y -= 2 * style.ItemSpacing.y;

    sliderSize.y = std::max(MinHeight, sliderSize.y);

    ImVec2 graphSize;
    graphSize.x = placementInfo.Size.x;
    graphSize.x -= sliderSize.x;
    graphSize.x -= 2 * style.WindowPadding.x;
    graphSize.x -= 2 * style.ItemSpacing.x;

    graphSize.y = sliderSize.y;
    graphSize.y -= style.ItemSpacing.y * (m_channelData.size() - 1);
    graphSize.y /= m_channelData.size();

    // We use negative numbers for the scale so the slider goes up for more sensitivity,
    // which is a bit more intuitive
    //
    ImGuiExtensions::K4AVSliderFloat("##MicrophoneScale", sliderSize, &m_microphoneScale, -1.0f, -0.1f, "Scale");

    ImGui::SameLine();

    ImGui::BeginGroup();
    for (auto &channelData : m_channelData)
    {
        channelData.Show(graphSize, -m_microphoneScale);
    }
    ImGui::EndGroup();
}

const char *K4AAudioWindow::GetTitle() const
{
    return m_title.c_str();
}

void K4AAudioWindow::ProcessNewData()
{
    if (!m_listener)
    {
        return;
    }

    m_listener->ProcessFrames([this](K4AMicrophoneFrame *frame, const size_t frameCount) {
        for (size_t frameId = 0; frameId < frameCount; frameId++)
        {
            for (size_t channelId = 0; channelId < K4AMicrophoneFrame::ChannelCount; channelId++)
            {
                m_channelData[channelId].AddSample(frame[frameId].Channel[channelId]);
            }
        }

        return frameCount;
    });

    if (m_listener->GetStatus() != SoundIoErrorNone)
    {
        std::stringstream errorBuilder;
        errorBuilder << "Error while recording " << soundio_strerror(m_listener->GetStatus()) << "!";

        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
        m_listener.reset();
    }
    else if (m_listener->Overflowed())
    {
        K4AViewerErrorManager::Instance().SetErrorStatus("Warning: sound overflow detected!");
        m_listener->ClearOverflowed();
    }
}
