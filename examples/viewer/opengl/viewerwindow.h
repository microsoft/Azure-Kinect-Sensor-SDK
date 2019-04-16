// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef VIEWERWINDOW_H
#define VIEWERWINDOW_H

#include "k4aimgui_all.h"

#include "texture.h"

namespace viewer
{
class ViewerWindow
{
public:
    // ImGui's OpenGL/GLFW bindings use globals that make having multiple windows
    // not work properly, so the ViewerWindow is a singleton.
    //
    static ViewerWindow &Instance();

    // Initialize the window, setting its default title and dimensions.
    // You must call Initialize() before attempting to render to the window.
    //
    void Initialize(const char *windowTitle, int defaultWidth, int defaultHeight);

    // Tells the graphics framework to start a new frame.
    // Returns false if the application has been closed and we should exit.
    //
    bool BeginFrame();

    // Tells the graphics framework to finish rendering the current frame.
    //
    void EndFrame();

    // Get the current width of the window
    // Useful for figuring out how to size what you pass to ShowWindow().
    //
    inline int GetWidth() const
    {
        return m_windowWidth;
    }

    // Get the current height of the window.
    // Useful for figuring out how to size what you pass to ShowWindow().
    //
    inline int GetHeight() const
    {
        return m_windowHeight;
    }

    // Create a Texture with the specified dimensions that you can use to show image data
    //
    Texture CreateTexture(int width, int height);
    Texture CreateTexture(std::pair<int, int> dimensions);

    // Show a sub-window within the main window with the given name that shows the image stored in
    // texture at the provided position.  The image will be stretched (maintaining aspect ratio) to
    // fill maxSize.
    //
    void ShowTexture(const char *name, const Texture &texture, const ImVec2 &position, const ImVec2 &maxSize);

    ~ViewerWindow();

private:
    ViewerWindow() = default;

    GLFWwindow *m_window = nullptr;

    int m_windowWidth;
    int m_windowHeight;
};
} // namespace viewer

#endif