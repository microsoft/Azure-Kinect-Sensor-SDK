// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AVIEWER_H
#define K4AVIEWER_H

// System headers
//
#include <memory>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//

struct K4AViewerOptions
{
    bool HighDpi = false;
};

namespace k4aviewer
{
class K4AViewer
{
public:
    explicit K4AViewer(const K4AViewerOptions &args);
    ~K4AViewer();

    void Run();

    K4AViewer(const K4AViewer &) = delete;
    K4AViewer(const K4AViewer &&) = delete;
    K4AViewer &operator=(const K4AViewer &) = delete;
    K4AViewer &operator=(const K4AViewer &&) = delete;

private:
    void ShowMainMenuBar();

    void SetHighDpi();

    void ShowErrorOverlay();

    GLFWwindow *m_window;
    bool m_showDeveloperOptions = false;
    bool m_showDemoWindow = false;
    bool m_showStyleEditor = false;
    bool m_showMetricsWindow = false;
    bool m_showPerfCounters = false;
};
} // namespace k4aviewer

#endif
