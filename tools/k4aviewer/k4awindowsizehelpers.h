// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AWINDOWSIZEHELPERS_H
#define K4AWINDOWSIZEHELPERS_H

// System headers
//

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//

namespace k4aviewer
{
inline float GetStandardVerticalSliderWidth()
{
    // Width of the slider is 1 character, plus half the normal padding
    //
    return ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.x;
}

inline float GetTitleBarHeight()
{
    return ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2;
}

inline float GetDefaultButtonHeight()
{
    return ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2 + ImGui::GetStyle().ItemSpacing.y;
}

// Gets the maximum dimensions that an image of size imageDimensions can be scaled to in order
// to fit in a window with imageMaxSize available space while maintaining its aspect ratio.
// imageMaxSize is expected to include space for window padding, but does not account for the
// title bar or any any potential other widgets in your window, so you'll need to subtract those
// from imageMaxSize, if applicable.
//
inline ImVec2 GetMaxImageSize(const ImVec2 imageDimensions, const ImVec2 imageMaxSize)
{
    const float sourceAspectRatio = imageDimensions.x / imageDimensions.y;

    const float verticalPadding = ImGui::GetStyle().WindowPadding.y * 2;
    const float horizontalPadding = ImGui::GetStyle().WindowPadding.x * 2;

    const float horizontalMaxSize = imageMaxSize.x - horizontalPadding;
    const float verticalMaxSize = imageMaxSize.y - verticalPadding;
    ImVec2 displayDimensions;
    if (horizontalMaxSize / sourceAspectRatio <= verticalMaxSize)
    {
        displayDimensions.x = horizontalMaxSize;
        displayDimensions.y = horizontalMaxSize / sourceAspectRatio;
    }
    else
    {
        displayDimensions.x = verticalMaxSize * sourceAspectRatio;
        displayDimensions.y = verticalMaxSize;
    }

    displayDimensions.x = std::max(1.f, displayDimensions.x);
    displayDimensions.y = std::max(1.f, displayDimensions.y);

    return displayDimensions;
}
} // namespace k4aviewer

#endif
