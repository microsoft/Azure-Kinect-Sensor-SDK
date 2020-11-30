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
inline std::pair<int, int> GetColorDimensions(int mode_index)
{
    if (K4ASourceSelectionDockControl::SelectedDevice != -1)
    {
        k4a::device device = k4a::device::open(static_cast<uint32_t>(K4ASourceSelectionDockControl::SelectedDevice));
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
    else
    {
        throw std::logic_error("No device selected!");
    }
}

// Gets the dimensions of the depth images that the depth camera will produce for a
// given depth mode
//
inline std::pair<int, int> GetDepthDimensions(int mode_index)
{
    if (K4ASourceSelectionDockControl::SelectedDevice != -1)
        {
        k4a::device device = k4a::device::open(static_cast<uint32_t>(K4ASourceSelectionDockControl::SelectedDevice));
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
    else
    {
        throw std::logic_error("No device selected!");
    }
}

// Gets the range of values that we expect to see from the depth camera
// when using a given depth mode, in millimeters
//
inline std::pair<uint16_t, uint16_t> GetDepthModeRange(int mode_index)
{
    if (K4ASourceSelectionDockControl::SelectedDevice != -1)
        {
        k4a::device device = k4a::device::open(static_cast<uint32_t>(K4ASourceSelectionDockControl::SelectedDevice));
        
        switch (mode_index)
        {
        case 5: // K4A_DEPTH_MODE_PASSIVE_IR
        default:
            std::vector<k4a_depth_mode_info_t> depth_modes = device.get_depth_modes();
            size_t size = depth_modes.size();
            if (size > 0 && mode_index < size)
            {
                k4a_depth_mode_info_t mode = depth_modes[mode_index];
                return { (uint16_t)mode.min_range, (uint16_t)mode.max_range };
            }
            else
            {
                throw std::logic_error("Invalid depth mode!");
            }
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
inline std::pair<uint16_t, uint16_t> GetIrLevels(int mode_index)
{
    k4a::device device = k4a::device::open(static_cast<uint32_t>(K4ASourceSelectionDockControl::SelectedDevice));
    if (mode_index == 0) // K4A_DEPTH_MODE_OFF
    {
        throw std::logic_error("Invalid depth mode!");
    }
    else if (mode_index == 5) // K4A_DEPTH_MODE_PASSIVE_IR
    {
        if (K4ASourceSelectionDockControl::SelectedDevice != -1)
        {
            std::vector<k4a_depth_mode_info_t> depth_modes = device.get_depth_modes();
            size_t size = depth_modes.size();
            if (size > 0 && mode_index < size)
            {
                k4a_depth_mode_info_t mode = depth_modes[mode_index];
                return { (uint16_t)mode.min_range, (uint16_t)mode.max_range };
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
    else
    {
        return { (uint16_t)0, (uint16_t)1000 };
    }
}
} // namespace k4aviewer

#endif