/** \file modes.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef MODES_H
#define MODES_H

#ifdef __cplusplus
extern "C" {
#endif

/** Depth sensor capture modes.
 *
 * \remarks
 * See the hardware specification for additional details on the field of view, and supported frame rates
 * for each mode.
 *
 * \remarks
 * NFOV and WFOV denote Narrow and Wide Field Of View configurations.
 *
 * \remarks
 * Binned modes reduce the captured camera resolution by combining adjacent sensor pixels into a bin.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
// Be sure to update k4a_depth_mode_to_string in k4a.c if enum values are added.
typedef enum
{
    K4A_DEPTH_MODE_OFF = 0,        /**< Depth sensor will be turned off with this setting. */
    K4A_DEPTH_MODE_NFOV_2X2BINNED, /**< Depth captured at 320x288. Passive IR is also captured at 320x288. */
    K4A_DEPTH_MODE_NFOV_UNBINNED,  /**< Depth captured at 640x576. Passive IR is also captured at 640x576. */
    K4A_DEPTH_MODE_WFOV_2X2BINNED, /**< Depth captured at 512x512. Passive IR is also captured at 512x512. */
    K4A_DEPTH_MODE_WFOV_UNBINNED,  /**< Depth captured at 1024x1024. Passive IR is also captured at 1024x1024. */
    K4A_DEPTH_MODE_PASSIVE_IR,     /**< Passive IR only, captured at 1024x1024. */
    K4A_DEPTH_MODE_COUNT,          /**< Must be last entry. */
} k4a_depth_mode_t;

/** Color sensor resolutions.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
// Be sure to update k4a_color_resolution_to_string in k4a.c if enum values are added.
typedef enum
{
    K4A_COLOR_RESOLUTION_OFF = 0, /**< Color camera will be turned off with this setting */
    K4A_COLOR_RESOLUTION_720P,    /**< 1280 * 720  16:9 */
    K4A_COLOR_RESOLUTION_1080P,   /**< 1920 * 1080 16:9 */
    K4A_COLOR_RESOLUTION_1440P,   /**< 2560 * 1440 16:9 */
    K4A_COLOR_RESOLUTION_1536P,   /**< 2048 * 1536 4:3  */
    K4A_COLOR_RESOLUTION_2160P,   /**< 3840 * 2160 16:9 */
    K4A_COLOR_RESOLUTION_3072P,   /**< 4096 * 3072 4:3  */
    K4A_COLOR_RESOLUTION_COUNT,   /**< Must be last entry. */
} k4a_color_resolution_t;

/** Color and depth sensor frame rate.
 *
 * \remarks
 * This enumeration is used to select the desired frame rate to operate the cameras. The actual
 * frame rate may vary slightly due to dropped data, synchronization variation between devices,
 * clock accuracy, or if the camera exposure priority mode causes reduced frame rate.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
// Be sure to update k4a_fps_to_string in k4a.c if enum values are added.
typedef enum
{
    K4A_FRAMES_PER_SECOND_0 = 0,   /**< 0 FPS */
    K4A_FRAMES_PER_SECOND_5 = 5,   /**< 5 FPS */
    K4A_FRAMES_PER_SECOND_15 = 15, /**< 15 FPS */
    K4A_FRAMES_PER_SECOND_30 = 30, /**< 30 FPS */
} k4a_fps_t;

/** Return the image width and height for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static bool k4a_convert_resolution_to_width_height(k4a_color_resolution_t resolution,
                                                          uint32_t *width_out,
                                                          uint32_t *height_out)
{
    if (width_out == NULL || height_out == NULL)
    {
        return false;
    }

    uint32_t width = 0;
    uint32_t height = 0;
    bool resolutionFound = true;

    switch (resolution)
    {
    case K4A_COLOR_RESOLUTION_OFF:
        width = 0;
        height = 0;
        break;
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
        resolutionFound = false;
        break;
    }

    if (resolutionFound)
    {
        *width_out = width;
        *height_out = height;
    }

    return resolutionFound;
}

/** Return the image width for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static uint32_t k4a_color_resolution_width(k4a_color_resolution_t resolution)
{
    uint32_t width = 0;
    uint32_t height = 0;
    k4a_convert_resolution_to_width_height(resolution, &width, &height);
    return width;
}

/** Return the image height for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static uint32_t k4a_color_resolution_height(k4a_color_resolution_t resolution)
{
    uint32_t width = 0;
    uint32_t height = 0;
    k4a_convert_resolution_to_width_height(resolution, &width, &height);
    return height;
}

/** Return the camera field of view for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static bool k4a_convert_resolution_to_fov(k4a_color_resolution_t resolution,
                                                 float *horizontal_fov,
                                                 float *vertical_fov)
{
    if (horizontal_fov == NULL || vertical_fov == NULL)
    {
        return false;
    }

    float hor_fov = 0.0f;
    float ver_fov = 0.0f;
    bool resolutionFound = true;

    switch (resolution)
    {
    case K4A_COLOR_RESOLUTION_OFF:
        hor_fov = 0;
        ver_fov = 0;
        break;
    case K4A_COLOR_RESOLUTION_720P:
        hor_fov = 90.0f;
        ver_fov = 59.0f;
        break;
    case K4A_COLOR_RESOLUTION_1080P:
        hor_fov = 90.0f;
        ver_fov = 59.0f;
        break;
    case K4A_COLOR_RESOLUTION_1440P:
        hor_fov = 90.0f;
        ver_fov = 59.0f;
        break;
    case K4A_COLOR_RESOLUTION_1536P:
        hor_fov = 90.0f;
        ver_fov = 74.3f;
        break;
    case K4A_COLOR_RESOLUTION_2160P:
        hor_fov = 90.0f;
        ver_fov = 59.0f;
        break;
    case K4A_COLOR_RESOLUTION_3072P:
        hor_fov = 90.0f;
        ver_fov = 74.3f;
        break;
    default:
        resolutionFound = false;
        break;
    }

    if (resolutionFound)
    {
        *horizontal_fov = hor_fov;
        *vertical_fov = ver_fov;
    }

    return resolutionFound;
}

/** Return the color camera horizontal fov for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static float k4a_color_resolution_horizontal_fov(k4a_color_resolution_t resolution)
{
    float horizontal_fov = 0;
    float vertical_fov = 0;
    k4a_convert_resolution_to_fov(resolution, &horizontal_fov, &vertical_fov);
    return horizontal_fov;
}

/** Return the color camera vertical fov for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static float k4a_color_resolution_vertical_fov(k4a_color_resolution_t resolution)
{
    float horizontal_fov = 0;
    float vertical_fov = 0;
    k4a_convert_resolution_to_fov(resolution, &horizontal_fov, &vertical_fov);
    return vertical_fov;
}

/** Return the image width and height for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static bool k4a_convert_depth_mode_to_width_height(k4a_depth_mode_t mode_id,
                                                          uint32_t *width_out,
                                                          uint32_t *height_out)
{
    if (width_out == NULL || height_out == NULL)
    {
        return false;
    }

    uint32_t width = 0;
    uint32_t height = 0;
    bool modeFound = true;

    switch (mode_id)
    {
    case K4A_DEPTH_MODE_OFF:
        width = 0;
        height = 0;
        break;
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
        modeFound = false;
        break;
    }

    if (modeFound)
    {
        *width_out = width;
        *height_out = height;
    }

    return modeFound;
}

/** Return the image width for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static uint32_t k4a_depth_mode_width(k4a_depth_mode_t mode_id)
{
    uint32_t width = 0;
    uint32_t height = 0;
    k4a_convert_depth_mode_to_width_height(mode_id, &width, &height);
    return width;
}

/** Return the image height for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static uint32_t k4a_depth_mode_height(k4a_depth_mode_t mode_id)
{
    uint32_t width = 0;
    uint32_t height = 0;
    k4a_convert_depth_mode_to_width_height(mode_id, &width, &height);
    return height;
}

/** Return the depth camera field of view for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static bool k4a_convert_depth_mode_to_fov(k4a_depth_mode_t mode_id, float *horizontal_fov, float *vertical_fov)
{
    if (horizontal_fov == NULL || vertical_fov == NULL)
    {
        return false;
    }

    float hor_fov = 0.0f;
    float ver_fov = 0.0f;
    bool modeFound = true;

    switch (mode_id)
    {
    case K4A_DEPTH_MODE_OFF:
        hor_fov = 0;
        ver_fov = 0;
        break;
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        hor_fov = 75.0f;
        ver_fov = 65.0f;
        break;
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        hor_fov = 75.0f;
        ver_fov = 65.0f;
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        hor_fov = 120.0f;
        ver_fov = 120.0f;
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        hor_fov = 120.0f;
        ver_fov = 120.0f;
        break;
    case K4A_DEPTH_MODE_PASSIVE_IR:
        hor_fov = 120.0f;
        ver_fov = 120.0f;
        break;
    default:
        modeFound = false;
        break;
    }

    if (modeFound)
    {
        *horizontal_fov = hor_fov;
        *vertical_fov = ver_fov;
    }

    return modeFound;
}

/** Return the depth camera horizontal fov for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static float k4a_depth_mode_horizontal_fov(k4a_depth_mode_t mode_id)
{
    float horizontal_fov = 0;
    float vertical_fov = 0;
    k4a_convert_depth_mode_to_fov(mode_id, &horizontal_fov, &vertical_fov);
    return horizontal_fov;
}

/** Return the depth camera vertical fov for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static float k4a_depth_mode_vertical_fov(k4a_depth_mode_t mode_id)
{
    float horizontal_fov = 0;
    float vertical_fov = 0;
    k4a_convert_depth_mode_to_fov(mode_id, &horizontal_fov, &vertical_fov);
    return vertical_fov;
}

/** Return the min and max depth range for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static bool k4a_convert_depth_mode_to_min_max_range(k4a_depth_mode_t mode_id,
                                                           uint32_t *min_range,
                                                           uint32_t *max_range)
{
    if (min_range == NULL || max_range == NULL)
    {
        return false;
    }

    uint32_t min_range_temp = 0;
    uint32_t max_range_temp = 0;
    bool modeFound = true;

    switch (mode_id)
    {
    case K4A_DEPTH_MODE_OFF:
        min_range_temp = 0;
        max_range_temp = 0;
        break;
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        min_range_temp = 500;
        max_range_temp = 5800;
        break;
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        min_range_temp = 500;
        max_range_temp = 4000;
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        min_range_temp = 250;
        max_range_temp = 3000;
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        min_range_temp = 250;
        max_range_temp = 2500;
        break;
    case K4A_DEPTH_MODE_PASSIVE_IR:
        min_range_temp = 0;
        max_range_temp = 0;
        break;
    default:
        modeFound = false;
        break;
    }

    if (modeFound)
    {
        *min_range = min_range_temp;
        *max_range = max_range_temp;
    }

    return modeFound;
}

/** Return the min depth range for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static uint32_t k4a_depth_mode_min_range(k4a_depth_mode_t mode_id)
{
    uint32_t min_range = 0;
    uint32_t max_range = 0;
    k4a_convert_depth_mode_to_width_height(mode_id, &min_range, &max_range);
    return min_range;
}

/** Return the max depth range for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
inline static uint32_t k4a_depth_mode_max_range(k4a_depth_mode_t mode_id)
{
    uint32_t min_range = 0;
    uint32_t max_range = 0;
    k4a_convert_depth_mode_to_width_height(mode_id, &min_range, &max_range);
    return max_range;
}

#ifdef __cplusplus
}
#endif

#endif /* MODES_H */