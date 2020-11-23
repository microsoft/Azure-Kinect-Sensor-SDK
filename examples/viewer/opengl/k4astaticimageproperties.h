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
inline std::pair<int, int> GetColorDimensions(k4a_device_t device_handle, int mode_index)
{
    // TODO: comment
    int color_mode_count;
    k4a_result_t color_mode_count_result = k4a_device_get_color_mode_count(device_handle, &color_mode_count);
    if (color_mode_count_result == K4A_RESULT_SUCCEEDED)
    {
        if (mode_index >= 0 && mode_index < color_mode_count)
        {
            k4a_color_mode_info_t color_mode_info;
            k4a_result_t color_mode_result = k4a_device_get_color_mode(device_handle, mode_index, &color_mode_info);
            // TODO: improve error handling
            if (color_mode_result == K4A_RESULT_SUCCEEDED)
            {
                return { (int)color_mode_info.width, (int)color_mode_info.height };
            }
            else
            {
                throw std::logic_error("Invalid color dimensions value!");
            }
        }
        else
        {
            throw std::logic_error("Invalid color dimensions value!");
        }
    }
    else
    {
        throw std::logic_error("Invalid device handle!");
    }
}

// Gets the dimensions of the depth images that the depth camera will produce for a
// given depth mode
//
inline std::pair<int, int> GetDepthDimensions(k4a_device_t device_handle, int mode_index)
{
    int depth_mode_count;
    k4a_result_t depth_mode_count_result = k4a_device_get_depth_mode_count(device_handle, &depth_mode_count);
    if (depth_mode_count_result == K4A_RESULT_SUCCEEDED)
    {
        if (mode_index >= 0 && mode_index < depth_mode_count)
        {
            k4a_depth_mode_info_t depth_mode_info;
            k4a_result_t color_mode_result = k4a_device_get_depth_mode(device_handle, mode_index, &depth_mode_info);
            // TODO: improve error handling
            if (color_mode_result == K4A_RESULT_SUCCEEDED)
            {
                return { (int)depth_mode_info.width, (int)depth_mode_info.height };
            }
            else
            {
                throw std::logic_error("Invalid depth dimensions value!");
            }
        }
        else
        {
            throw std::logic_error("Invalid depth dimensions value!");
        }
    }
    else
    {
        throw std::logic_error("Invalid device handle!");
    }
}

// Gets the range of values that we expect to see from the depth camera
// when using a given depth mode, in millimeters
//
inline std::pair<uint16_t, uint16_t> GetDepthModeRange(k4a_device_t device_handle, int mode_index)
{
    switch (mode_index)
    {
    case 5: // K4A_DEPTH_MODE_PASSIVE_IR
    default:
        int depth_mode_count;
        k4a_result_t depth_mode_count_result = k4a_device_get_depth_mode_count(device_handle, &depth_mode_count);
        if (depth_mode_count_result == K4A_RESULT_SUCCEEDED)
        {
            if (mode_index >= 0 && mode_index < depth_mode_count)
            {
                k4a_depth_mode_info_t depth_mode_info;
                k4a_result_t depth_mode_result = k4a_device_get_depth_mode(device_handle, mode_index, &depth_mode_info);
                // TODO: improve error handling
                if (depth_mode_result == K4A_RESULT_SUCCEEDED)
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
                throw std::logic_error("Invalid depth mode!");
            }
        }
        else
        {
            throw std::logic_error("Invalid device handle!");
        }
    }
}

// Gets the expected min/max IR brightness levels that we expect to see
// from the IR camera when using a given depth mode
//
inline std::pair<uint16_t, uint16_t> GetIrLevels(k4a_device_t device_handle, int mode_index)
{
    if (mode_index == 0) // K4A_DEPTH_MODE_OFF
    {
        throw std::logic_error("Invalid depth mode!");
    }
    else if (mode_index == 5) // K4A_DEPTH_MODE_PASSIVE_IR
    {
        int depth_mode_count;
        k4a_result_t depth_mode_count_result = k4a_device_get_depth_mode_count(device_handle, &depth_mode_count);
        if (depth_mode_count_result == K4A_RESULT_SUCCEEDED)
        {
            k4a_depth_mode_info_t depth_mode_info;
            k4a_result_t result = k4a_device_get_depth_mode(device_handle, mode_index, &depth_mode_info);
            // TODO: improve error handling
            if (result == K4A_RESULT_SUCCEEDED)
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
            throw std::logic_error("Invalid device handle!");
        }
    }
    else
    {
        return { (uint16_t)0, (uint16_t)1000 };
    }

}
} // namespace viewer

#endif