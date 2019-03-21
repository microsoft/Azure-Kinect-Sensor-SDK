// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "viewerwindow.h"

#include <iostream>

#include "viewerutil.h"

using namespace viewer;

namespace
{

// A callback that we give to OpenGL so we can get notified about any errors that might
// occur when they happen.  Notifications relevant to performance tuning also come in
// through this callback.
//
void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar *message,
                            void *userParam)
{
    (void)userParam;

    // Some of the performance messages are a bit noisy, so we want to drop those
    // to reduce noise in the log.
    //
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

    std::cerr << "OpenGL debug message:" << std::endl
              << "  source: " << source << std::endl
              << "  type:   " << type << std::endl
              << "  id:     " << id << std::endl
              << "  sev:    " << severity << std::endl
              << "  len:    " << length << std::endl
              << "  msg:    " << message << std::endl
              << "---------------------------" << std::endl;
}
} // namespace

ViewerWindow &ViewerWindow::Instance()
{
    static ViewerWindow viewerWindow;
    return viewerWindow;
}

void ViewerWindow::Initialize(const char *windowTitle, int defaultWidth, int defaultHeight)
{
    std::cout << "Started initializing OpenGL..." << std::endl;

    if (m_window != nullptr)
    {
        throw std::logic_error("Attempted to double-initialize the window!");
    }

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    m_windowWidth = defaultWidth;
    m_windowHeight = defaultHeight;

    // Start the window
    //
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, windowTitle, nullptr, nullptr);

    glfwMakeContextCurrent(m_window);

    // Enable vsync (cap framerate at the display's refresh rate)
    //
    glfwSwapInterval(1);

    // Initialize OpenGL
    //
    if (gl3wInit())
    {
        throw std::runtime_error("Failed to initialize OpenGL!");
    }

    // Turn on OpenGL debugging.  While not strictly necessary, this makes it much easier
    // to track down OpenGL errors when they occur.
    //
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    // Initialize ImGui
    //
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ImGui style settings
    //
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0.0f;

    // By default, ImGui tries to save the previous window layout to disk.
    // That doesn't really make sense for this application, so we want to
    // disable saving the window layout.
    //
    ImGui::GetIO().IniFilename = nullptr;

    CheckOpenGLErrors();

    std::cout << "Finished initializing OpenGL." << std::endl;
}

ViewerWindow::~ViewerWindow()
{
    // ImGui will assert if we throw without having called Render, which
    // obscures the actual error message, so we call it here just in case.
    //
    ImGui::Render();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool ViewerWindow::BeginFrame()
{
    if (m_window == nullptr)
    {
        // You need to call ViewerWindow::Initialize first.
        //
        throw std::logic_error("Attempted to use uninitialized window!");
    }

    // glfwWindowShouldClose will start returning true when the user
    // clicks the close button on the title bar.
    //
    if (glfwWindowShouldClose(m_window))
    {
        return false;
    }

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    return true;
}

void ViewerWindow::EndFrame()
{
    ImGui::Render();

    glfwMakeContextCurrent(m_window);
    glfwGetFramebufferSize(m_window, &m_windowWidth, &m_windowHeight);
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
    glfwPollEvents();

    CheckOpenGLErrors();
}

Texture ViewerWindow::CreateTexture(int width, int height)
{
    return Texture(width, height);
}

Texture ViewerWindow::CreateTexture(std::pair<int, int> dimensions)
{
    return Texture(dimensions.first, dimensions.second);
}

void ViewerWindow::ShowTexture(const char *name, const Texture &texture, const ImVec2 &position, const ImVec2 &maxSize)
{
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(maxSize);

    if (ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        // Figure out how big we can make the image
        //
        ImVec2 imageSize(static_cast<float>(texture.Width()), static_cast<float>(texture.Height()));
        ImVec2 imageMaxSize = maxSize;
        imageMaxSize.y -= GetTitleBarHeight();

        imageSize = GetMaxImageSize(imageSize, imageMaxSize);

        ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture.Name())), imageSize);
    }
    ImGui::End();
}
