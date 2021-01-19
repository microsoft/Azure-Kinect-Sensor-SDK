#ifndef MODES_H
#define MODES_H

#ifdef __cplusplus
extern "C" {
#endif

// TODO: rename k4a_depth_mode_unused_t?


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
    K4A_DEPTH_MODE_COUNT,
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
    K4A_COLOR_RESOLUTION_COUNT,
} k4a_color_resolution_t;



/** Color and depth sensor frame rate.
 *
 * \remarks
 * This enumeration is used to select the desired frame rate to operate the cameras. The actual
 * frame rate may vary slightly due to dropped data, synchronization variation between devices,
 * clock accuracy, or if the camera exposure priority mode causes reduced frame rate.
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">modes.h (include k4ainternal/modes.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
// Be sure to update k4a_fps_to_string in k4a.c if enum values are added.
typedef enum
{
    K4A_FRAMES_PER_SECOND_5 = 0, /**< 5 FPS */
    K4A_FRAMES_PER_SECOND_15,    /**< 15 FPS */
    K4A_FRAMES_PER_SECOND_30,    /**< 30 FPS */
    K4A_FRAMES_PER_SECOND_COUNT,
} k4a_fps_t;



struct _device_color_modes
{
    uint32_t width;
    uint32_t height;
    k4a_image_format_t native_format;
    float horizontal_fov;
    float vertical_fov;
    int min_fps;
    int max_fps;
} device_color_modes[] = { { 0, 0, K4A_IMAGE_FORMAT_COLOR_MJPG, 0, 0, 0, 0 }, // color mode will be turned off
                           { 1280, 720, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30 },
                           { 1920, 1080, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30 },
                           { 2560, 1440, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30 },
                           { 2048, 1536, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 74.3f, 5, 30 },
                           { 3840, 2160, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30 },
                           { 4096, 3072, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 74.3f, 5, 30 } };

struct _device_depth_modes
{
    bool passive_ir_only;
    uint32_t width;
    uint32_t height;
    k4a_image_format_t native_format;
    float horizontal_fov;
    float vertical_fov;
    int min_fps;
    int max_fps;
    int min_range;
    int max_range;
} device_depth_modes[] = { { false, 0, 0, K4A_IMAGE_FORMAT_DEPTH16, 0.0f, 0.0f, 0, 0, 0, 0 }, // depth mode will be
                                                                                              // turned off
                           { false, 320, 288, K4A_IMAGE_FORMAT_DEPTH16, 75.0f, 65.0f, 5, 30, 500, 5800 },
                           { false, 640, 576, K4A_IMAGE_FORMAT_DEPTH16, 75.0f, 65.0f, 5, 30, 500, 4000 },
                           { false, 512, 512, K4A_IMAGE_FORMAT_DEPTH16, 120.0f, 120.0f, 5, 30, 250, 3000 },
                           { false, 1024, 1024, K4A_IMAGE_FORMAT_DEPTH16, 120.0f, 120.0f, 5, 30, 250, 2500 },
                           { true, 1024, 1024, K4A_IMAGE_FORMAT_DEPTH16, 120.0f, 120.0f, 5, 30, 0, 100 } };

struct _device_fps_modes
{
    int fps;
} device_fps_modes[] = { { 5 }, { 15 }, { 30 } };

#ifdef __cplusplus
}
#endif

#endif /* MODES_H */