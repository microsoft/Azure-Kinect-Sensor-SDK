// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AIMAGESIZES_H
#define K4AIMAGESIZES_H

#include <exception>
#include <utility>

#include <k4a/k4a.hpp>

namespace viewer
{

// Gets the dimensions of the color images that the color camera will produce for a
// given color resolution
//
inline std::pair<int, int> GetColorDimensions(const k4a_color_resolution_t resolution)
{
    switch (resolution)
    {
    case K4A_COLOR_RESOLUTION_720P:
        return { 1280, 720 };
    case K4A_COLOR_RESOLUTION_2160P:
        return { 3840, 2160 };
    case K4A_COLOR_RESOLUTION_1440P:
        return { 2560, 1440 };
    case K4A_COLOR_RESOLUTION_1080P:
        return { 1920, 1080 };
    case K4A_COLOR_RESOLUTION_3072P:
        return { 4096, 3072 };
    case K4A_COLOR_RESOLUTION_1536P:
        return { 2048, 1536 };

    default:
        throw std::logic_error("Invalid color dimensions value!");
    }
}

// Gets the dimensions of the depth images that the depth camera will produce for a
// given depth mode
//
inline std::pair<int, int> GetDepthDimensions(const k4a_depth_mode_t depthMode)
{
    switch (depthMode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        return { 320, 288 };
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        return { 640, 576 };
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        return { 512, 512 };
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        return { 1024, 1024 };
    case K4A_DEPTH_MODE_PASSIVE_IR:
        return { 1024, 1024 };

    default:
        throw std::logic_error("Invalid depth dimensions value!");
    }
}

// Gets the range of values that we expect to see from the depth camera
// when using a given depth mode, in millimeters
//
inline std::pair<uint16_t, uint16_t> GetDepthModeRange(const k4a_depth_mode_t depthMode)
{
    switch (depthMode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        return { (uint16_t)500, (uint16_t)5800 };
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        return { (uint16_t)500, (uint16_t)4000 };
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        return { (uint16_t)250, (uint16_t)3000 };
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        return { (uint16_t)250, (uint16_t)2500 };

    case K4A_DEPTH_MODE_PASSIVE_IR:
    default:
        throw std::logic_error("Invalid depth mode!");
    }
}

// Gets the expected min/max IR brightness levels that we expect to see
// from the IR camera when using a given depth mode
//
inline std::pair<uint16_t, uint16_t> GetIrLevels(const k4a_depth_mode_t depthMode)
{
    switch (depthMode)
    {
    case K4A_DEPTH_MODE_PASSIVE_IR:
        return { (uint16_t)0, (uint16_t)100 };

    case K4A_DEPTH_MODE_OFF:
        throw std::logic_error("Invalid depth mode!");

    default:
        return { (uint16_t)0, (uint16_t)1000 };
    }
}
} // namespace viewer

#endif