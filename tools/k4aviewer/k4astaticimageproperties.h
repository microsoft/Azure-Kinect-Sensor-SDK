// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AIMAGESIZES_H
#define K4AIMAGESIZES_H

#include <exception>
#include <utility>

#include <k4a/k4a.hpp>
#include "k4asourceselectiondockcontrol.h"

namespace k4aviewer
{

// Gets the dimensions of the color images that the color camera will produce for a
// given color resolution
//
inline std::pair<int, int> GetColorDimensions(k4a_color_mode_info_t color_mode_info)
{
    if (K4ASourceSelectionDockControl::SelectedDevice != -1)
    {
        return { (int)color_mode_info.width, (int)color_mode_info.height };
    }
    else
    {
        throw std::logic_error("No device selected!");
    }
}

// Gets the dimensions of the depth images that the depth camera will produce for a
// given depth mode
//
inline std::pair<int, int> GetDepthDimensions(k4a_depth_mode_info_t depth_mode_info)
{
    if (K4ASourceSelectionDockControl::SelectedDevice != -1)
    {
        return { (int)depth_mode_info.width, (int)depth_mode_info.height };
    }
    else
    {
        throw std::logic_error("No device selected!");
    }
}

// Gets the range of values that we expect to see from the depth camera
// when using a given depth mode, in millimeters
//
inline std::pair<uint16_t, uint16_t> GetDepthModeRange(k4a_depth_mode_info_t depth_mode_info)
{
    if (K4ASourceSelectionDockControl::SelectedDevice != -1)
    {
        if (!depth_mode_info.passive_ir_only)
        {
            return { (uint16_t)depth_mode_info.min_range, (uint16_t)depth_mode_info.max_range };
        }
        else
        {
            throw std::logic_error("Invalid depth mode!");
        }
    }
    else
    {
        throw std::logic_error("No device selected!");
    }
}

// Gets the expected min/max IR brightness levels that we expect to see
// from the IR camera when using a given depth mode
//
inline std::pair<uint16_t, uint16_t> GetIrLevels(k4a_depth_mode_info_t depth_mode_info)
{
    if (depth_mode_info.mode_id == 0) // K4A_DEPTH_MODE_OFF
    {
        throw std::logic_error("Invalid depth mode!");
    }
    else if (depth_mode_info.passive_ir_only) // K4A_DEPTH_MODE_PASSIVE_IR
    {
        if (K4ASourceSelectionDockControl::SelectedDevice != -1)
        {
            return { (uint16_t)depth_mode_info.min_range, (uint16_t)depth_mode_info.max_range };
        }
        else
        {
            throw std::logic_error("No device selected!");
        }
    }
    else
    {
        return { (uint16_t)0, (uint16_t)1000 };
    }
}
} // namespace k4aviewer

#endif