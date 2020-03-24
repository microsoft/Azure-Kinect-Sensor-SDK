/** \file COMMON.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef COMMON_H
#define COMMON_H

#include <k4a/k4atypes.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _guid_t
{
    uint8_t id[16];
} guid_t;

#define K4A_IMU_SAMPLE_RATE 1666 // +/- 2%

#define MAX_FPS_IN_MS (33) // 30 FPS

#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define STRINGIFY(string) #string

// Clock tick runs 90kHz and convert sec to micro sec
#define K4A_90K_HZ_TICK_TO_USEC(x) ((uint64_t)(x)*100 / 9)
#define K4A_USEC_TO_90K_HZ_TICK(x) ((x)*9 / 100)

#define MAX_SERIAL_NUMBER_LENGTH                                                                                       \
    (13 * 2) // Current schema is for 12 digits plus NULL, the extra size is in case that grows in the future.

#define HZ_TO_PERIOD_MS(Hz) (1000 / Hz)
#define HZ_TO_PERIOD_US(Hz) (1000000 / Hz)
#define HZ_TO_PERIOD_NS(Hz) (1000000000 / Hz)

inline static uint32_t k4a_convert_fps_to_uint(k4a_fps_t fps)
{
    uint32_t fps_int;
    switch (fps)
    {
    case K4A_FRAMES_PER_SECOND_5:
        fps_int = 5;
        break;
    case K4A_FRAMES_PER_SECOND_15:
        fps_int = 15;
        break;
    case K4A_FRAMES_PER_SECOND_30:
        fps_int = 30;
        break;
    default:
        assert(0);
        fps_int = 0;
        break;
    }
    return fps_int;
}

inline static bool k4a_convert_resolution_to_width_height(k4a_color_resolution_t resolution,
                                                          uint32_t *width_out,
                                                          uint32_t *height_out)
{
    uint32_t width = 0;
    uint32_t height = 0;
    switch (resolution)
    {
    case K4A_COLOR_RESOLUTION_720P:
        width = 1280;
        height = 720;
        break;
    case K4A_COLOR_RESOLUTION_1080P:
        width = 1920;
        height = 1080;
        break;
    case K4A_COLOR_RESOLUTION_1440P:
        width = 2560;
        height = 1440;
        break;
    case K4A_COLOR_RESOLUTION_1536P:
        width = 2048;
        height = 1536;
        break;
    case K4A_COLOR_RESOLUTION_2160P:
        width = 3840;
        height = 2160;
        break;
    case K4A_COLOR_RESOLUTION_3072P:
        width = 4096;
        height = 3072;
        break;
    default:
        return false;
    }

    if (width_out != NULL)
        *width_out = width;
    if (height_out != NULL)
        *height_out = height;
    return true;
}

inline static bool k4a_convert_depth_mode_to_width_height(k4a_depth_mode_t mode,
                                                          uint32_t *width_out,
                                                          uint32_t *height_out)
{
    uint32_t width = 0;
    uint32_t height = 0;
    switch (mode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        width = 320;
        height = 288;
        break;
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        width = 640;
        height = 576;
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        width = 512;
        height = 512;
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        width = 1024;
        height = 1024;
        break;
    case K4A_DEPTH_MODE_PASSIVE_IR:
        width = 1024;
        height = 1024;
        break;
    default:
        return false;
    }

    if (width_out != NULL)
        *width_out = width;
    if (height_out != NULL)
        *height_out = height;
    return true;
}

inline static bool k4a_is_version_greater_or_equal(k4a_version_t *fw_version_l, k4a_version_t *fw_version_r)
{
    typedef enum
    {
        FW_OK,
        FW_TOO_LOW,
        FW_UNKNOWN
    } fw_check_state_t;

    fw_check_state_t fw = FW_UNKNOWN;

    // Check major version
    if (fw_version_l->major > fw_version_r->major)
    {
        fw = FW_OK;
    }
    else if (fw_version_l->major < fw_version_r->major)
    {
        fw = FW_TOO_LOW;
    }

    // Check minor version
    if (fw == FW_UNKNOWN)
    {
        if (fw_version_l->minor > fw_version_r->minor)
        {
            fw = FW_OK;
        }
        else if (fw_version_l->minor < fw_version_r->minor)
        {
            fw = FW_TOO_LOW;
        }
    }

    // Check iteration version
    if (fw == FW_UNKNOWN)
    {
        fw = FW_TOO_LOW;
        if (fw_version_l->iteration >= fw_version_r->iteration)
        {
            fw = FW_OK;
        }
    }

    return (fw == FW_OK);
}
#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
