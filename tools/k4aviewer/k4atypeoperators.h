// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ATYPEOPERATORS_H
#define K4ATYPEOPERATORS_H

// System headers
//
#include <ostream>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//

namespace k4aviewer
{
// Comparison operators
//
bool operator<(const k4a_version_t &a, const k4a_version_t &b);

// Output operators for K4A API enum types
//
std::ostream &operator<<(std::ostream &s, const k4a_result_t &val);
std::ostream &operator<<(std::ostream &s, const k4a_buffer_result_t &val);
std::ostream &operator<<(std::ostream &s, const k4a_wait_result_t &val);
std::ostream &operator<<(std::ostream &s, const k4a_color_control_command_t &val);

std::ostream &operator<<(std::ostream &s, const k4a_wired_sync_mode_t &val);
std::istream &operator>>(std::istream &s, k4a_wired_sync_mode_t &val);

std::ostream &operator<<(std::ostream &s, const k4a_fps_t &val);
std::istream &operator>>(std::istream &s, k4a_fps_t &val);

std::ostream &operator<<(std::ostream &s, const k4a_depth_mode_t &val);
std::istream &operator>>(std::istream &s, k4a_depth_mode_t &val);

std::ostream &operator<<(std::ostream &s, const k4a_color_resolution_t &val);
std::istream &operator>>(std::istream &s, k4a_color_resolution_t &val);

std::ostream &operator<<(std::ostream &s, const k4a_image_format_t &val);
std::istream &operator>>(std::istream &s, k4a_image_format_t &val);

} // namespace k4aviewer

#endif
