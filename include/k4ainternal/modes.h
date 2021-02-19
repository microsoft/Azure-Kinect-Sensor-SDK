/** \file modes.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef MODES_H
#define MODES_H

#include <k4a/k4atypes.h>

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

/** Get count of device depth modes.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_get_device_depth_modes_count();

/** Get count of device color modes.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_get_device_color_modes_count();

/** Get count of device fps modes.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_get_device_fps_modes_count();

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
k4a_depth_mode_info_t k4a_get_device_depth_mode(uint32_t mode_index);

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
k4a_color_mode_info_t k4a_get_device_color_mode(uint32_t mode_index);

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
k4a_fps_mode_info_t k4a_get_device_fps_mode(uint32_t mode_index);

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
uint32_t k4a_convert_fps_to_uint(k4a_fps_t fps);

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
k4a_fps_t k4a_convert_uint_to_fps(uint32_t fps_in);

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
                                            uint32_t *height_out);

/** Return the image width for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_color_resolution_width(k4a_color_resolution_t resolution);

/** Return the image height for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_color_resolution_height(k4a_color_resolution_t resolution);

/** Return the camera field of view for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
bool k4a_convert_resolution_to_fov(k4a_color_resolution_t resolution, float *horizontal_fov, float *vertical_fov);

/** Return the color camera horizontal fov for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
float k4a_color_resolution_horizontal_fov(k4a_color_resolution_t resolution);

/** Return the color camera vertical fov for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
float k4a_color_resolution_vertical_fov(k4a_color_resolution_t resolution);

/** Return the image width and height for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
bool k4a_convert_depth_mode_to_width_height(k4a_depth_mode_t mode_id, uint32_t *width_out, uint32_t *height_out);

/** Return the image width for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_depth_mode_width(k4a_depth_mode_t mode_id);

/** Return the image height for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_depth_mode_height(k4a_depth_mode_t mode_id);

/** Return the depth camera field of view for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
bool k4a_convert_depth_mode_to_fov(k4a_depth_mode_t mode_id, float *horizontal_fov, float *vertical_fov);

/** Return the depth camera horizontal fov for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
float k4a_depth_mode_horizontal_fov(k4a_depth_mode_t mode_id);

/** Return the depth camera vertical fov for the corresponding k4a_color_resolution_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
float k4a_depth_mode_vertical_fov(k4a_depth_mode_t mode_id);

/** Return the min and max depth range for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
bool k4a_convert_depth_mode_to_min_max_range(k4a_depth_mode_t mode_id, uint32_t *min_range, uint32_t *max_range);

/** Return the min depth range for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_depth_mode_min_range(k4a_depth_mode_t mode_id);

/** Return the max depth range for the corresponding k4a_depth_mode_t enum.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
uint32_t k4a_depth_mode_max_range(k4a_depth_mode_t mode_id);

#ifdef __cplusplus
}
#endif

#endif /* MODES_H */