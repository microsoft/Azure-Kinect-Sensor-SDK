// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/modes.h>
#include <k4a/k4atypes.h>

// Dependent libraries

// System dependencies

#ifdef __cplusplus
extern "C" {
#endif

// Create a static array of color modes. Let the struct_size and variable fields be 0 for now.
// These values refer specifically to Azure Kinect device.
static const k4a_color_mode_info_t device_color_modes[] = {
    { 0, K4A_ABI_VERSION, K4A_COLOR_RESOLUTION_OFF, 0, 0, K4A_IMAGE_FORMAT_COLOR_MJPG, 0, 0, 0, 0 },
    { 0, K4A_ABI_VERSION, K4A_COLOR_RESOLUTION_720P, 1280, 720, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30 },
    { 0, K4A_ABI_VERSION, K4A_COLOR_RESOLUTION_1080P, 1920, 1080, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30 },
    { 0, K4A_ABI_VERSION, K4A_COLOR_RESOLUTION_1440P, 2560, 1440, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30 },
    { 0, K4A_ABI_VERSION, K4A_COLOR_RESOLUTION_1536P, 2048, 1536, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 74.3f, 5, 30 },
    { 0, K4A_ABI_VERSION, K4A_COLOR_RESOLUTION_2160P, 3840, 2160, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30 },
    { 0, K4A_ABI_VERSION, K4A_COLOR_RESOLUTION_3072P, 4096, 3072, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 74.3f, 5, 30 }
};

// An alias so the lines below will not get too long.
#define DEPTH16 K4A_IMAGE_FORMAT_DEPTH16

// Create a static array of depth modes. Let the struct_size and variable fields be 0 for now.
// These values refer specifically to Azure Kinect device.
static const k4a_depth_mode_info_t device_depth_modes[] = {
    { 0, K4A_ABI_VERSION, K4A_DEPTH_MODE_OFF, false, 0, 0, DEPTH16, 0.0f, 0.0f, 0, 0, 0, 0 },
    { 0, K4A_ABI_VERSION, K4A_DEPTH_MODE_NFOV_2X2BINNED, false, 320, 288, DEPTH16, 75.0f, 65.0f, 5, 30, 500, 5800 },
    { 0, K4A_ABI_VERSION, K4A_DEPTH_MODE_NFOV_UNBINNED, false, 640, 576, DEPTH16, 75.0f, 65.0f, 5, 30, 500, 4000 },
    { 0, K4A_ABI_VERSION, K4A_DEPTH_MODE_WFOV_2X2BINNED, false, 512, 512, DEPTH16, 120.0f, 120.0f, 5, 30, 250, 3000 },
    { 0, K4A_ABI_VERSION, K4A_DEPTH_MODE_WFOV_UNBINNED, false, 1024, 1024, DEPTH16, 120.0f, 120.0f, 5, 30, 250, 2500 },
    { 0, K4A_ABI_VERSION, K4A_DEPTH_MODE_PASSIVE_IR, true, 1024, 1024, DEPTH16, 120.0f, 120.0f, 5, 30, 0, 100 }
};

// Create a static array of fps modes. Let the struct_size and variable fields be 0 for now.
// These values refer specifically to Azure Kinect device.
static const k4a_fps_mode_info_t device_fps_modes[] = { { 0, K4A_ABI_VERSION, K4A_FRAMES_PER_SECOND_0, 0 },
                                                        { 0, K4A_ABI_VERSION, K4A_FRAMES_PER_SECOND_5, 5 },
                                                        { 0, K4A_ABI_VERSION, K4A_FRAMES_PER_SECOND_15, 15 },
                                                        { 0, K4A_ABI_VERSION, K4A_FRAMES_PER_SECOND_30, 30 } };

/** Get count of device depth modes.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_get_device_depth_modes_count()
{
    return (uint32_t)(sizeof(device_depth_modes) / sizeof(device_depth_modes[0]));
}

/** Get count of device color modes.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_get_device_color_modes_count()
{
    return (uint32_t)(sizeof(device_color_modes) / sizeof(device_color_modes[0]));
}

/** Get count of device fps modes.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_get_device_fps_modes_count()
{
    return (uint32_t)(sizeof(device_fps_modes) / sizeof(device_fps_modes[0]));
}

/** Get device depth mode for the given index.
 *
 * \param mode_index The index of the depth mode to get.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
k4a_depth_mode_info_t k4a_get_device_depth_mode(uint32_t mode_index)
{
    k4a_depth_mode_info_t depth_mode;
    if (mode_index < k4a_get_device_depth_modes_count())
        depth_mode = device_depth_modes[mode_index];

    depth_mode.struct_size = (uint32_t)sizeof(k4a_depth_mode_info_t);
    return depth_mode;
}

/** Get device color mode for the given index.
 *
 * \param mode_index The index of the depth mode to get.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
k4a_color_mode_info_t k4a_get_device_color_mode(uint32_t mode_index)
{
    k4a_color_mode_info_t color_mode;
    if (mode_index < k4a_get_device_color_modes_count())
        color_mode = device_color_modes[mode_index];

    color_mode.struct_size = (uint32_t)sizeof(k4a_color_mode_info_t);
    return color_mode;
}

/** Get device fps mode for the given index.
 *
 * \param mode_index The index of the depth mode to get.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
k4a_fps_mode_info_t k4a_get_device_fps_mode(uint32_t mode_index)
{
    k4a_fps_mode_info_t fps_mode;
    if (mode_index < k4a_get_device_fps_modes_count())
        fps_mode = device_fps_modes[mode_index];

    fps_mode.struct_size = (uint32_t)sizeof(k4a_fps_mode_info_t);
    return fps_mode;
}

/** Convert k4a_fps_t enum to the actual frame rate value.
 *
 * \remarks
 * If the fps enum is not valid, then 0 fps is returned.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_convert_fps_to_uint(k4a_fps_t fps)
{
    uint32_t fpsValue = 0;

    // Search device_color_modes array for the given resolution.
    for (size_t n = 0; n < sizeof(device_fps_modes) / sizeof(device_fps_modes[0]); ++n)
    {
        if ((k4a_fps_t)(device_fps_modes[n].mode_id) == fps)
        {
            fpsValue = (uint32_t)(device_fps_modes[n].fps);
            break;
        }
    }

    return fpsValue;
}

/** Convert frame rate value to the corresponding k4a_fps_t enum.
 *
 * \remarks
 * If the fps value does not correspond to a k4a_fps_t enum, then K4A_FRAME_PER_SECOND_0 is returned.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
k4a_fps_t k4a_convert_uint_to_fps(uint32_t fps_in)
{
    k4a_fps_t fps_t = K4A_FRAMES_PER_SECOND_0;

    // Search device_color_modes array for the given resolution.
    for (size_t n = 0; n < sizeof(device_fps_modes) / sizeof(device_fps_modes[0]); ++n)
    {
        if ((uint32_t)device_fps_modes[n].fps == fps_in)
        {
            fps_t = (k4a_fps_t)(device_fps_modes[n].mode_id);
            break;
        }
    }

    return fps_t;
}

/** Return the image width and height for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
bool k4a_convert_resolution_to_width_height(k4a_color_resolution_t resolution,
                                            uint32_t *width_out,
                                            uint32_t *height_out)
{
    bool resolutionFound = false;

    if (width_out == NULL || height_out == NULL)
    {
        return resolutionFound;
    }

    // Search device_color_modes array for the given resolution.
    for (size_t n = 0; n < sizeof(device_color_modes) / sizeof(device_color_modes[0]); ++n)
    {
        if (device_color_modes[n].mode_id == (uint32_t)resolution)
        {
            *width_out = device_color_modes[n].width;
            *height_out = device_color_modes[n].height;
            resolutionFound = true;
            break;
        }
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
uint32_t k4a_color_resolution_width(k4a_color_resolution_t resolution)
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
uint32_t k4a_color_resolution_height(k4a_color_resolution_t resolution)
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
bool k4a_convert_resolution_to_fov(k4a_color_resolution_t resolution, float *horizontal_fov, float *vertical_fov)
{
    bool resolutionFound = false;

    if (horizontal_fov == NULL || vertical_fov == NULL)
    {
        return resolutionFound;
    }

    // Search device_color_modes array for the given resolution.
    for (size_t n = 0; n < sizeof(device_color_modes) / sizeof(device_color_modes[0]); ++n)
    {
        if (device_color_modes[n].mode_id == (uint32_t)resolution)
        {
            *horizontal_fov = device_color_modes[n].horizontal_fov;
            *vertical_fov = device_color_modes[n].vertical_fov;
            resolutionFound = true;
            break;
        }
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
float k4a_color_resolution_horizontal_fov(k4a_color_resolution_t resolution)
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
float k4a_color_resolution_vertical_fov(k4a_color_resolution_t resolution)
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
bool k4a_convert_depth_mode_to_width_height(k4a_depth_mode_t mode_id, uint32_t *width_out, uint32_t *height_out)
{
    bool modeFound = false;

    if (width_out == NULL || height_out == NULL)
    {
        return modeFound;
    }

    // Search device_depth_modes array for the given resolution.
    for (size_t n = 0; n < sizeof(device_depth_modes) / sizeof(device_depth_modes[0]); ++n)
    {
        if (device_depth_modes[n].mode_id == (uint32_t)mode_id)
        {
            *width_out = device_depth_modes[n].width;
            *height_out = device_depth_modes[n].height;
            modeFound = true;
            break;
        }
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
uint32_t k4a_depth_mode_width(k4a_depth_mode_t mode_id)
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
uint32_t k4a_depth_mode_height(k4a_depth_mode_t mode_id)
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
bool k4a_convert_depth_mode_to_fov(k4a_depth_mode_t mode_id, float *horizontal_fov, float *vertical_fov)
{
    bool modeFound = false;

    if (horizontal_fov == NULL || vertical_fov == NULL)
    {
        return modeFound;
    }

    // Search device_depth_modes array for the given resolution.
    for (size_t n = 0; n < sizeof(device_depth_modes) / sizeof(device_depth_modes[0]); ++n)
    {
        if (device_depth_modes[n].mode_id == (uint32_t)mode_id)
        {
            *horizontal_fov = device_depth_modes[n].horizontal_fov;
            *vertical_fov = device_depth_modes[n].vertical_fov;
            modeFound = true;
            break;
        }
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
float k4a_depth_mode_horizontal_fov(k4a_depth_mode_t mode_id)
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
float k4a_depth_mode_vertical_fov(k4a_depth_mode_t mode_id)
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
bool k4a_convert_depth_mode_to_min_max_range(k4a_depth_mode_t mode_id, uint32_t *min_range, uint32_t *max_range)
{
    bool modeFound = false;

    if (min_range == NULL || max_range == NULL)
    {
        return modeFound;
    }

    // Search device_depth_modes array for the given resolution.
    for (size_t n = 0; n < sizeof(device_depth_modes) / sizeof(device_depth_modes[0]); ++n)
    {
        if (device_depth_modes[n].mode_id == (uint32_t)mode_id)
        {
            *min_range = device_depth_modes[n].min_range;
            *max_range = device_depth_modes[n].max_range;
            modeFound = true;
            break;
        }
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
uint32_t k4a_depth_mode_min_range(k4a_depth_mode_t mode_id)
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
uint32_t k4a_depth_mode_max_range(k4a_depth_mode_t mode_id)
{
    uint32_t min_range = 0;
    uint32_t max_range = 0;
    k4a_convert_depth_mode_to_width_height(mode_id, &min_range, &max_range);
    return max_range;
}

#ifdef __cplusplus
}
#endif
