// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AIMGUI_ALL_H
#define K4AIMGUI_ALL_H

// Some of the dear imgui implementation headers must be included in a specific order, which can
// lead to really odd build breaks if you include one of those headers to get something out of it
// without including the rest in a header whose includes don't already include all of those
// dependencies from some other include they already have.
//
// This file includes all dear imgui-related headers that have ordering constraints; if you need
// access to symbols from any of these headers, include this header rather than directly including
// those headers.
//

// Some of these headers end up including windows.h, which defines the min/max macros, which conflict
// with std::min and std::max, which we're using because they're portable.  This macro modifies the
// behavior of windows.h to not define those macros so we can avoid the conflict.
//
#define NOMINMAX

// Clang parses doxygen-style comments in your source and checks for doxygen syntax errors.
// Unfortunately, some of our external dependencies have doxygen syntax errors in their
// headers and clang looks at them when we include them here, so we need to shut off those
// warnings on these headers.
//
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// For disabling buttons, which has not yet been promoted to the public API
//
#include <imgui_internal.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
