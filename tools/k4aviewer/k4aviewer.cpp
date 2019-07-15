// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aviewer.h"

// System headers
//
#include <fstream>
#include <sstream>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4aaudiomanager.h"
#include "k4alogdockcontrol.h"
#include "k4asourceselectiondockcontrol.h"
#include "k4aviewererrormanager.h"
#include "k4aviewerutil.h"
#include "k4awindowmanager.h"
#include "perfcounter.h"

using namespace k4aviewer;

namespace
{
// Background color of app.  Black.
//
const ImVec4 ClearColor(0.01f, 0.01f, 0.01f, 1.0f);

constexpr int GlfwFailureExitCode = -1;

constexpr float HighDpiScaleFactor = 2.0f;

void LogGlfwError(const int error, const char *msg)
{
    std::ofstream errorLogger;
    errorLogger.open("k4aviewer.err", std::ofstream::out | std::ofstream::app);
    errorLogger << "Glfw error [" << error << "]: " << msg << std::endl;
    errorLogger.close();
}

} // namespace

#ifdef DEBUG
#define K4AVIEWER_ENABLE_OPENGL_DEBUGGING
#endif

#ifdef K4AVIEWER_ENABLE_OPENGL_DEBUGGING
void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar *message,
                            void *userParam)
{
    (void)userParam;

    constexpr GLuint noisyMessages[] = {
        131185, // Event that says a texture was loaded into memory
        131169, // Event that says a buffer was allocated
    };

    for (GLuint noisyMessageId : noisyMessages)
    {
        if (id == noisyMessageId)
        {
            return;
        }
    }

    std::ofstream msgLogger;
    msgLogger.open("k4aviewer.log", std::ofstream::out | std::ofstream::app);
    msgLogger << "OpenGL debug message:" << std::endl
              << "  source: " << source << std::endl
              << "  type:   " << type << std::endl
              << "  id:     " << id << std::endl
              << "  sev:    " << severity << std::endl
              << "  len:    " << length << std::endl
              << "  msg:    " << message << std::endl
              << "---------------------------" << std::endl;

    msgLogger.close();
}
#endif

K4AViewer::K4AViewer(const K4AViewerArgs &args)
{
    // Setup window
    glfwSetErrorCallback(LogGlfwError);
    if (!glfwInit())
    {
        // Couldn't initialize the graphics library, which means we're not going to get far.
        //
        LogGlfwError(0, "glfwInit failed!");
        exit(GlfwFailureExitCode);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(1440, 900, "Azure Kinect Viewer", nullptr, nullptr);
    if (!m_window)
    {
        LogGlfwError(0, "glfwCreateWindow failed!");
        exit(GlfwFailureExitCode);
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync
    gl3wInit();

#ifdef K4AVIEWER_ENABLE_OPENGL_DEBUGGING
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Setup style
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0.0f;

    // Disable saving window layout
    ImGui::GetIO().IniFilename = nullptr;

    if (args.HighDpi)
    {
        SetHighDpi();
    }

    const int audioInitStatus = K4AAudioManager::Instance().Initialize();
    if (audioInitStatus != SoundIoErrorNone)
    {
        std::stringstream errorBuilder;
        errorBuilder << "Failed to initialize audio backend: " << soundio_strerror(audioInitStatus) << "!";
        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str().c_str());
    }

    K4AWindowManager::Instance().PushLeftDockControl(std14::make_unique<K4ASourceSelectionDockControl>());
    K4AWindowManager::Instance().PushBottomDockControl(std14::make_unique<K4ALogDockControl>());
}

K4AViewer::~K4AViewer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void K4AViewer::Run()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        // Start the ImGui frame
        //
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ShowMainMenuBar();

        K4AWindowManager::Instance().ShowAll();

        ShowErrorOverlay();

        if (m_showDemoWindow)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }

        if (m_showStyleEditor)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            ImGui::Begin("Style editor", &m_showStyleEditor);
            ImGui::ShowStyleEditor();
            ImGui::End();
        }

        if (m_showMetricsWindow)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            ImGui::ShowMetricsWindow(&m_showMetricsWindow);
        }

        if (m_showPerfCounters)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            PerfCounterManager::ShowPerfWindow(&m_showPerfCounters);
        }

        // Finalize/render frame
        //
        ImGui::Render();
        int displayW;
        int displayH;
        glfwMakeContextCurrent(m_window);
        glfwGetFramebufferSize(m_window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        K4AWindowManager::Instance().SetGLWindowSize(ImVec2(float(displayW), float(displayH)));
        glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

void K4AViewer::ShowMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Settings"))
        {
            ShowViewerOptionMenuItem("Show log dock", ViewerOption::ShowLogDock);
            ShowViewerOptionMenuItem("Show info overlay", ViewerOption::ShowInfoPane);

            if (K4AViewerSettingsManager::Instance().GetViewerOption(ViewerOption::ShowInfoPane))
            {
                ShowViewerOptionMenuItem("Show framerate", ViewerOption::ShowFrameRateInfo);
            }

            ShowViewerOptionMenuItem("Show developer options", ViewerOption::ShowDeveloperOptions);

            ImGui::Separator();

            if (ImGui::MenuItem("Load default settings"))
            {
                K4AViewerSettingsManager::Instance().SetDefaults();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit"))
            {
                glfwSetWindowShouldClose(m_window, true);
            }
            ImGui::EndMenu();
        }

        if (K4AViewerSettingsManager::Instance().GetViewerOption(ViewerOption::ShowDeveloperOptions))
        {
            if (ImGui::BeginMenu("Developer"))
            {
                ImGui::MenuItem("Show demo window", nullptr, &m_showDemoWindow);
                ImGui::MenuItem("Show style editor", nullptr, &m_showStyleEditor);
                ImGui::MenuItem("Show metrics window", nullptr, &m_showMetricsWindow);
                ImGui::MenuItem("Show perf counters", nullptr, &m_showPerfCounters);

                ImGui::EndMenu();
            }
        }

        K4AWindowManager::Instance().SetMenuBarHeight(ImGui::GetWindowSize().y);
        ImGui::EndMainMenuBar();
    }
}

void K4AViewer::SetHighDpi()
{
    ImGui::GetStyle().ScaleAllSizes(HighDpiScaleFactor);

    // ImGui doesn't automatically scale fonts, so we have to do that ourselves
    //
    ImFontConfig fontConfig;
    constexpr float defaultFontSize = 13.0f;
    fontConfig.SizePixels = defaultFontSize * HighDpiScaleFactor;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    int w;
    int h;
    glfwGetWindowSize(m_window, &w, &h);
    w = static_cast<int>(w * HighDpiScaleFactor);
    h = static_cast<int>(h * HighDpiScaleFactor);
    glfwSetWindowSize(m_window, w, h);
}

void K4AViewer::ShowErrorOverlay()
{
    if (K4AViewerErrorManager::Instance().IsErrorSet())
    {
        constexpr char errorPopupTitle[] = "Error!";
        ImGui::OpenPopup(errorPopupTitle);
        ImGui::BeginPopupModal(errorPopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("%s", K4AViewerErrorManager::Instance().GetErrorMessage().c_str());

        if (ImGui::Button("Dismiss"))
        {
            K4AViewerErrorManager::Instance().PopError();
        }

        ImGui::EndPopup();
    }
}

void K4AViewer::ShowViewerOptionMenuItem(const char *msg, ViewerOption option)
{
    auto &settings = K4AViewerSettingsManager::Instance();
    bool isSet = settings.GetViewerOption(option);

    if (ImGui::MenuItem(msg, nullptr, isSet))
    {
        settings.SetViewerOption(option, !isSet);
    }
}