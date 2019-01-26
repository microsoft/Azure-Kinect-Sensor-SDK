if (NOT TARGET imgui)
    set(imguiextern_SOURCE_DIR src)
    add_library(imgui STATIC
                ${imguiextern_SOURCE_DIR}/imgui.cpp
                ${imguiextern_SOURCE_DIR}/imgui_demo.cpp
                ${imguiextern_SOURCE_DIR}/imgui_draw.cpp
                ${imguiextern_SOURCE_DIR}/imgui_widgets.cpp
                ${imguiextern_SOURCE_DIR}/examples/imgui_impl_glfw.cpp
                ${imguiextern_SOURCE_DIR}/examples/imgui_impl_opengl3.cpp
                ${imguiextern_SOURCE_DIR}/examples/libs/gl3w/GL/gl3w.c
    )

    target_include_directories(imgui PUBLIC
        ${imguiextern_SOURCE_DIR}
        ${imguiextern_SOURCE_DIR}/examples
        ${imguiextern_SOURCE_DIR}/examples/libs/gl3w
        ${imguiextern_SOURCE_DIR}/examples/libs/glfw/include
    )
else()
    message(STATUS "imgui is already a target. Skipping adding it twice")
endif()

add_library(imgui::imgui ALIAS imgui)
