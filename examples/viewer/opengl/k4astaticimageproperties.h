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
inline std::pair<int, int> GetColorDimensions(k4a::device device, int mode_index)
{
    std::vector<k4a_color_mode_info_t> color_modes = device.get_color_modes();
    size_t size = color_modes.size();
    if (size > 0 && mode_index < size)
    {
        k4a_color_mode_info_t mode = color_modes[mode_index];
        return { (int)mode.width, (int)mode.height };
    }
    else
    {
        throw std::logic_error("Invalid color mode!");
    }
}

// Gets the dimensions of the depth images that the depth camera will produce for a
// given depth mode
//
inline std::pair<int, int> GetDepthDimensions(k4a::device device, int mode_index)
{
    std::vector<k4a_depth_mode_info_t> depth_modes = device.get_depth_modes();
    size_t size = depth_modes.size();
    if (size > 0 && mode_index < size)
    {
        k4a_depth_mode_info_t mode = depth_modes[mode_index];
        return { (int)mode.width, (int)mode.height };
    }
    else
    {
        throw std::logic_error("Invalid depth mode!");
    }
}

// Gets the range of values that we expect to see from the depth camera
// when using a given depth mode, in millimeters
//
inline std::pair<uint16_t, uint16_t> GetDepthModeRange(k4a::device device, int mode_index)
{
    switch (mode_index)
    {
    case 5: // K4A_DEPTH_MODE_PASSIVE_IR
    default:
        std::vector<k4a_depth_mode_info_t> depth_modes = device.get_depth_modes();
        size_t size = depth_modes.size();
        if (size > 0 && mode_index < size)
        {
            k4a_depth_mode_info_t mode = depth_modes[mode_index];
            return { mode.min_range, mode.max_range };
        }
        else
        {
            throw std::logic_error("Invalid depth mode!");
        }
    }
}

// Gets the expected min/max IR brightness levels that we expect to see
// from the IR camera when using a given depth mode
//
inline std::pair<uint16_t, uint16_t> GetIrLevels(k4a::device device, int mode_index)
{
    if (mode_index == 0) // K4A_DEPTH_MODE_OFF
    {
        throw std::logic_error("Invalid depth mode!");
    }
    else if (mode_index == 5) // K4A_DEPTH_MODE_PASSIVE_IR
    {
        std::vector<k4a_depth_mode_info_t> depth_modes = device.get_depth_modes();
        size_t size = depth_modes.size();
        if (size > 0 && mode_index < size)
        {
            k4a_depth_mode_info_t mode = depth_modes[mode_index];
            return { mode.min_range, mode.max_range };
        }
        else
        {
            throw std::logic_error("Invalid depth mode!");
        }
    }
    else
    {
        return { (uint16_t)0, (uint16_t)1000 };
    }

}
} // namespace viewer

#endif