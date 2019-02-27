// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AAUDIOWINDOW_H
#define K4AAUDIOWINDOW_H

// System headers
//
#include <array>
#include <memory>

// Library headers
//

// Project headers
//
#include "ik4avisualizationwindow.h"
#include "k4aaudiochanneldatagraph.h"
#include "k4amicrophonelistener.h"

namespace k4aviewer
{
class K4AAudioWindow : public IK4AVisualizationWindow
{
public:
    explicit K4AAudioWindow(std::string &&title, std::shared_ptr<K4AMicrophoneListener> listener);

    void Show(K4AWindowPlacementInfo placementInfo) override;
    const char *GetTitle() const override;

private:
    void ProcessNewData();

    std::string m_title;

    std::shared_ptr<K4AMicrophoneListener> m_listener;

    std::array<K4AAudioChannelDataGraph, K4AMicrophoneFrame::ChannelCount> m_channelData;

    float m_microphoneScale = -0.5f;
};
} // namespace k4aviewer

#endif
