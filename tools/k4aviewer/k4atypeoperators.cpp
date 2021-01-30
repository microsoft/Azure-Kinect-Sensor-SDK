// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// System headers
//
#include "k4atypeoperators.h"

// Library headers
//
#include <sstream>

// Project headers
//

namespace k4aviewer
{
bool operator<(const k4a_version_t &a, const k4a_version_t &b)
{
    if (a.major > b.major)
        return false;
    if (a.minor > b.minor)
        return false;
    if (a.iteration >= b.iteration)
        return false;
    return true;
}

// Define the output operator for result types
std::ostream &operator<<(std::ostream &s, const k4a_result_t &val)
{
    switch (val)
    {
    case K4A_RESULT_SUCCEEDED:
        return s << "K4A_RESULT_SUCCEEDED";
    case K4A_RESULT_FAILED:
        return s << "K4A_RESULT_FAILED";
    }
    return s;
}

std::ostream &operator<<(std::ostream &s, const k4a_wait_result_t &val)
{
    switch (val)
    {
    case K4A_WAIT_RESULT_SUCCEEDED:
        return s << "K4A_WAIT_RESULT_SUCCEEDED";
    case K4A_WAIT_RESULT_FAILED:
        return s << "K4A_WAIT_RESULT_FAILED";
    case K4A_WAIT_RESULT_TIMEOUT:
        return s << "K4A_WAIT_RESULT_TIMEOUT";
    }
    return s;
}

std::ostream &operator<<(std::ostream &s, const k4a_buffer_result_t &val)
{
    switch (val)
    {
    case K4A_BUFFER_RESULT_SUCCEEDED:
        return s << "K4A_BUFFER_RESULT_SUCCEEDED";
    case K4A_BUFFER_RESULT_FAILED:
        return s << "K4A_BUFFER_RESULT_FAILED";
    case K4A_BUFFER_RESULT_TOO_SMALL:
        return s << "K4A_BUFFER_RESULT_TOO_SMALL";
    }
    return s;
}

std::ostream &operator<<(std::ostream &s, const k4a_color_control_command_t &val)
{
    switch (val)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE:
        return s << "EXPOSURE_TIME_ABSOLUTE";
    case K4A_COLOR_CONTROL_BRIGHTNESS:
        return s << "BRIGHTNESS";
    case K4A_COLOR_CONTROL_CONTRAST:
        return s << "CONTRAST";
    case K4A_COLOR_CONTROL_SATURATION:
        return s << "SATURATION";
    case K4A_COLOR_CONTROL_SHARPNESS:
        return s << "SHARPNESS";
    case K4A_COLOR_CONTROL_WHITEBALANCE:
        return s << "WHITEBALANCE";
    case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION:
        return s << "BACKLIGHT_COMPENSATION";
    case K4A_COLOR_CONTROL_GAIN:
        return s << "GAIN";
    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY:
        return s << "POWERLINE_FREQUENCY";
    default:
        s << static_cast<int>(val);
    }
    return s;
}

namespace
{
constexpr char WiredSyncModeStandalone[] = "K4A_WIRED_SYNC_MODE_STANDALONE";
constexpr char WiredSyncModeMaster[] = "K4A_WIRED_SYNC_MODE_MASTER";
constexpr char WiredSyncModeSubordinate[] = "K4A_WIRED_SYNC_MODE_SUBORDINATE";
} // namespace

std::ostream &operator<<(std::ostream &s, const k4a_wired_sync_mode_t &val)
{
    switch (val)
    {
    case K4A_WIRED_SYNC_MODE_STANDALONE:
        s << WiredSyncModeStandalone;
        break;
    case K4A_WIRED_SYNC_MODE_MASTER:
        s << WiredSyncModeMaster;
        break;
    case K4A_WIRED_SYNC_MODE_SUBORDINATE:
        s << WiredSyncModeSubordinate;
        break;
    default:
        throw std::logic_error("Unrecognized sync mode");
    }
    return s;
}

std::istream &operator>>(std::istream &s, k4a_wired_sync_mode_t &val)
{
    std::string str;
    s >> str;
    if (str == WiredSyncModeStandalone)
    {
        val = K4A_WIRED_SYNC_MODE_STANDALONE;
    }
    else if (str == WiredSyncModeMaster)
    {
        val = K4A_WIRED_SYNC_MODE_MASTER;
    }
    else if (str == WiredSyncModeSubordinate)
    {
        val = K4A_WIRED_SYNC_MODE_SUBORDINATE;
    }
    else
    {
        s.setstate(std::ios::failbit);
    }
    return s;
}

namespace
{
constexpr char ImageFormatColorMJPG[] = "K4A_IMAGE_FORMAT_COLOR_MJPG";
constexpr char ImageFormatColorNV12[] = "K4A_IMAGE_FORMAT_COLOR_NV12";
constexpr char ImageFormatColorYUY2[] = "K4A_IMAGE_FORMAT_COLOR_YUY2";
constexpr char ImageFormatColorBGRA32[] = "K4A_IMAGE_FORMAT_COLOR_BGRA32";
constexpr char ImageFormatDepth16[] = "K4A_IMAGE_FORMAT_DEPTH16";
constexpr char ImageFormatIR16[] = "K4A_IMAGE_FORMAT_IR16";
constexpr char ImageFormatCustom[] = "K4A_IMAGE_FORMAT_CUSTOM";
} // namespace

std::ostream &operator<<(std::ostream &s, const k4a_image_format_t &val)
{
    switch (val)
    {
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
        s << ImageFormatColorMJPG;
        break;
    case K4A_IMAGE_FORMAT_COLOR_NV12:
        s << ImageFormatColorNV12;
        break;
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
        s << ImageFormatColorYUY2;
        break;
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        s << ImageFormatColorBGRA32;
        break;
    case K4A_IMAGE_FORMAT_DEPTH16:
        s << ImageFormatDepth16;
        break;
    case K4A_IMAGE_FORMAT_IR16:
        s << ImageFormatIR16;
        break;
    case K4A_IMAGE_FORMAT_CUSTOM:
        s << ImageFormatCustom;
        break;
    default:
        throw std::logic_error("Unrecognized image format");
    }
    return s;
}

std::istream &operator>>(std::istream &s, k4a_image_format_t &val)
{
    std::string str;
    s >> str;
    if (str == ImageFormatColorMJPG)
    {
        val = K4A_IMAGE_FORMAT_COLOR_MJPG;
    }
    else if (str == ImageFormatColorNV12)
    {
        val = K4A_IMAGE_FORMAT_COLOR_NV12;
    }
    else if (str == ImageFormatColorYUY2)
    {
        val = K4A_IMAGE_FORMAT_COLOR_YUY2;
    }
    else if (str == ImageFormatColorBGRA32)
    {
        val = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    }
    else if (str == ImageFormatDepth16)
    {
        val = K4A_IMAGE_FORMAT_DEPTH16;
    }
    else if (str == ImageFormatIR16)
    {
        val = K4A_IMAGE_FORMAT_IR16;
    }
    else if (str == ImageFormatCustom)
    {
        val = K4A_IMAGE_FORMAT_CUSTOM;
    }
    else
    {
        s.setstate(std::ios::failbit);
    }
    return s;
}

} // namespace k4aviewer
