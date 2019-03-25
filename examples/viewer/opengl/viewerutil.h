// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef VIEWERUTIL_H
#define VIEWERUTIL_H

#include <sstream>

#include "k4aimgui_all.h"

namespace viewer
{

// Throw an error if OpenGL has encountered an error.
//
inline void CheckOpenGLErrors()
{
    const GLenum glStatus = glGetError();
    if (glStatus != GL_NO_ERROR)
    {
        std::stringstream errorBuilder;
        errorBuilder << "OpenGL error: " << static_cast<int>(glStatus);
        throw std::runtime_error(errorBuilder.str().c_str());
    }
}

// Gets the height of the title bar, in pixels
//
inline float GetTitleBarHeight()
{
    return ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2;
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

    return displayDimensions;
}

} // namespace viewer

#endif