// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "k4ainternal/transformation.h"
#include <k4ainternal/logging.h>

k4a_result_t
transformation_get_mode_specific_camera_calibration(const k4a_calibration_camera_t *raw_camera_calibration,
                                                    const k4a_camera_calibration_mode_info_t *mode_info,
                                                    k4a_calibration_camera_t *mode_specific_camera_calibration,
                                                    bool pixelized_zero_centered_output)
{
    if (K4A_FAILED(K4A_RESULT_FROM_BOOL(mode_info->calibration_image_binned_resolution[0] > 0 &&
                                        mode_info->calibration_image_binned_resolution[1] > 0 &&
                                        mode_info->output_image_resolution[0] > 0 &&
                                        mode_info->output_image_resolution[1] > 0)))
    {
        LOG_ERROR("Expect calibration image binned resolution and output image resolution are larger than 0, actual "
                  "values are calibration_image_binned_resolution: (%d,%d), output_image_resolution: (%d,%d).",
                  mode_info->calibration_image_binned_resolution[0],
                  mode_info->calibration_image_binned_resolution[1],
                  mode_info->output_image_resolution[0],
                  mode_info->output_image_resolution[1]);
        return K4A_RESULT_FAILED;
    }

    memcpy(mode_specific_camera_calibration, raw_camera_calibration, sizeof(k4a_calibration_camera_t));

    k4a_calibration_intrinsic_parameters_t *params = &mode_specific_camera_calibration->intrinsics.parameters;

    float cx = params->param.cx * mode_info->calibration_image_binned_resolution[0];
    float cy = params->param.cy * mode_info->calibration_image_binned_resolution[1];
    float fx = params->param.fx * mode_info->calibration_image_binned_resolution[0];
    float fy = params->param.fy * mode_info->calibration_image_binned_resolution[1];

    cx -= mode_info->crop_offset[0];
    cy -= mode_info->crop_offset[1];

    if (pixelized_zero_centered_output == true)
    {
        // raw calibration is unitized and 0-cornered, i.e., principal point and focal length are divided by image
        // dimensions and coordinate (0,0) corresponds to the top left corner of the top left pixel. Convert to
        // pixelized and 0-centered OpenCV convention used in the SDK, i.e., principal point and focal length are not
        // normalized and (0,0) represents the center of the top left pixel.
        params->param.cx = cx - 0.5f;
        params->param.cy = cy - 0.5f;
        params->param.fx = fx;
        params->param.fy = fy;
    }
    else
    {
        params->param.cx = cx / mode_info->output_image_resolution[0];
        params->param.cy = cy / mode_info->output_image_resolution[1];
        params->param.fx = fx / mode_info->output_image_resolution[0];
        params->param.fy = fy / mode_info->output_image_resolution[1];
    }

    mode_specific_camera_calibration->resolution_width = (int)mode_info->output_image_resolution[0];
    mode_specific_camera_calibration->resolution_height = (int)mode_info->output_image_resolution[1];

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t
transformation_get_mode_specific_depth_camera_calibration(const k4a_calibration_camera_t *raw_camera_calibration,
                                                          const k4a_depth_mode_t depth_mode,
                                                          k4a_calibration_camera_t *mode_specific_camera_calibration)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, raw_camera_calibration == NULL);
    // TODO: Read this from calibration data instead of hardcoding.
    if (K4A_FAILED(K4A_RESULT_FROM_BOOL(raw_camera_calibration->resolution_width == 1024 &&
                                        raw_camera_calibration->resolution_height == 1024)))
    {
        LOG_ERROR("Unexpected raw camera calibration resolution (%d,%d), should be (%d,%d).",
                  raw_camera_calibration->resolution_width,
                  raw_camera_calibration->resolution_height,
                  1024,
                  1024);
        return K4A_RESULT_FAILED;
    }

    switch (depth_mode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 512, 512 }, { 96, 90 }, { 320, 288 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(raw_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 1024, 1024 }, { 192, 180 }, { 640, 576 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(raw_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 512, 512 }, { 0, 0 }, { 512, 512 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(raw_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
    case K4A_DEPTH_MODE_PASSIVE_IR:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 1024, 1024 }, { 0, 0 }, { 1024, 1024 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(raw_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    default:
    {
        return K4A_RESULT_FAILED;
    }
    }
}

k4a_result_t
transformation_get_mode_specific_color_camera_calibration(const k4a_calibration_camera_t *raw_camera_calibration,
                                                          const k4a_color_resolution_t color_resolution,
                                                          k4a_calibration_camera_t *mode_specific_camera_calibration)
{
    if (raw_camera_calibration->resolution_width * 9 / 16 == raw_camera_calibration->resolution_height)
    {
        // Legacy calibration uses 16:9 mode. If such a calibration is detected, convert to 4:3 mode. Keep calibration
        // unitized and 0-cornered. It will be converted to pixelized and 0-centered in a subsequent call to
        // transformation_get_mode_specific_camera_calibration().
        k4a_camera_calibration_mode_info_t mode_info = { { 4096, 2304 }, { 0, -384 }, { 4096, 3072 } };
        if (K4A_FAILED(TRACE_CALL(
                transformation_get_mode_specific_camera_calibration(raw_camera_calibration,
                                                                    &mode_info,
                                                                    mode_specific_camera_calibration,
                                                                    /* pixelized_zero_centered_output = */ false))))
        {
            return K4A_RESULT_FAILED;
        }
    }
    else if (raw_camera_calibration->resolution_width * 3 / 4 == raw_camera_calibration->resolution_height)
    {
        memcpy(mode_specific_camera_calibration, raw_camera_calibration, sizeof(k4a_calibration_camera_t));
    }
    else
    {
        LOG_ERROR("Unexpected aspect ratio %d:%d, should either be 16:9 or 4:3.",
                  raw_camera_calibration->resolution_width,
                  raw_camera_calibration->resolution_height);
        return K4A_RESULT_FAILED;
    }

    switch (color_resolution)
    {
    case K4A_COLOR_RESOLUTION_720P:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 1280, 960 }, { 0, 120 }, { 1280, 720 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(mode_specific_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    case K4A_COLOR_RESOLUTION_1080P:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 1920, 1440 }, { 0, 180 }, { 1920, 1080 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(mode_specific_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    case K4A_COLOR_RESOLUTION_1440P:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 2560, 1920 }, { 0, 240 }, { 2560, 1440 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(mode_specific_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    case K4A_COLOR_RESOLUTION_1536P:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 2048, 1536 }, { 0, 0 }, { 2048, 1536 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(mode_specific_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    case K4A_COLOR_RESOLUTION_2160P:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 3840, 2880 }, { 0, 360 }, { 3840, 2160 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(mode_specific_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    case K4A_COLOR_RESOLUTION_3072P:
    {
        k4a_camera_calibration_mode_info_t mode_info = { { 4096, 3072 }, { 0, 0 }, { 4096, 3072 } };
        return TRACE_CALL(
            transformation_get_mode_specific_camera_calibration(mode_specific_camera_calibration,
                                                                &mode_info,
                                                                mode_specific_camera_calibration,
                                                                /* pixelized_zero_centered_output = */ true));
    }
    default:
    {
        LOG_ERROR("Unexpected color resolution type %d.", color_resolution);
        return K4A_RESULT_FAILED;
    }
    }
}