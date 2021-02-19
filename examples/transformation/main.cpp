// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <string>
#include "transformation_helpers.h"
#include "turbojpeg.h"
using namespace std;

static bool point_cloud_color_to_depth(k4a_transformation_t transformation_handle,
                                       const k4a_image_t depth_image,
                                       const k4a_image_t color_image,
                                       std::string file_name)
{
    int depth_image_width_pixels = k4a_image_get_width_pixels(depth_image);
    int depth_image_height_pixels = k4a_image_get_height_pixels(depth_image);
    k4a_image_t transformed_color_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                 depth_image_width_pixels,
                                                 depth_image_height_pixels,
                                                 depth_image_width_pixels * 4 * (int)sizeof(uint8_t),
                                                 &transformed_color_image))
    {
        printf("Failed to create transformed color image\n");
        return false;
    }

    k4a_image_t point_cloud_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
                                                 depth_image_width_pixels,
                                                 depth_image_height_pixels,
                                                 depth_image_width_pixels * 3 * (int)sizeof(int16_t),
                                                 &point_cloud_image))
    {
        printf("Failed to create point cloud image\n");
        return false;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_transformation_color_image_to_depth_camera(transformation_handle,
                                                                               depth_image,
                                                                               color_image,
                                                                               transformed_color_image))
    {
        printf("Failed to compute transformed color image\n");
        return false;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_transformation_depth_image_to_point_cloud(transformation_handle,
                                                                              depth_image,
                                                                              K4A_CALIBRATION_TYPE_DEPTH,
                                                                              point_cloud_image))
    {
        printf("Failed to compute point cloud\n");
        return false;
    }

    tranformation_helpers_write_point_cloud(point_cloud_image, transformed_color_image, file_name.c_str());

    k4a_image_release(transformed_color_image);
    k4a_image_release(point_cloud_image);

    return true;
}

static bool point_cloud_depth_to_color(k4a_transformation_t transformation_handle,
                                       const k4a_image_t depth_image,
                                       const k4a_image_t color_image,
                                       std::string file_name)
{
    // transform color image into depth camera geometry
    int color_image_width_pixels = k4a_image_get_width_pixels(color_image);
    int color_image_height_pixels = k4a_image_get_height_pixels(color_image);
    k4a_image_t transformed_depth_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
                                                 color_image_width_pixels,
                                                 color_image_height_pixels,
                                                 color_image_width_pixels * (int)sizeof(uint16_t),
                                                 &transformed_depth_image))
    {
        printf("Failed to create transformed depth image\n");
        return false;
    }

    k4a_image_t point_cloud_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
                                                 color_image_width_pixels,
                                                 color_image_height_pixels,
                                                 color_image_width_pixels * 3 * (int)sizeof(int16_t),
                                                 &point_cloud_image))
    {
        printf("Failed to create point cloud image\n");
        return false;
    }

    if (K4A_RESULT_SUCCEEDED !=
        k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image))
    {
        printf("Failed to compute transformed depth image\n");
        return false;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_transformation_depth_image_to_point_cloud(transformation_handle,
                                                                              transformed_depth_image,
                                                                              K4A_CALIBRATION_TYPE_COLOR,
                                                                              point_cloud_image))
    {
        printf("Failed to compute point cloud\n");
        return false;
    }

    tranformation_helpers_write_point_cloud(point_cloud_image, color_image, file_name.c_str());

    k4a_image_release(transformed_depth_image);
    k4a_image_release(point_cloud_image);

    return true;
}

static k4a_result_t
get_device_mode_ids(k4a_device_t device, uint32_t *color_mode_id, uint32_t *depth_mode_id, uint32_t *fps_mode_id)
{
    // 1. declare device info and mode infos
    k4a_device_info_t device_info = { sizeof(k4a_device_info_t), K4A_ABI_VERSION, 0 };
    k4a_color_mode_info_t color_mode_info = { sizeof(k4a_color_mode_info_t), K4A_ABI_VERSION, 0 };
    k4a_depth_mode_info_t depth_mode_info = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, 0 };
    k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, 0 };

    // 2. initialize device capabilities
    bool hasDepthDevice = false;
    bool hasColorDevice = false;

    // 3. get the count of modes
    uint32_t color_mode_count = 0;
    uint32_t depth_mode_count = 0;
    uint32_t fps_mode_count = 0;

    // 4. get available modes from device info
    if (!k4a_device_get_info(device, &device_info) == K4A_RESULT_SUCCEEDED)
    {
        cout << "Failed to get device info" << endl;
        exit(-1);
    }

    hasDepthDevice = (device_info.capabilities.bitmap.bHasDepth == 1);
    hasColorDevice = (device_info.capabilities.bitmap.bHasColor == 1);

    if (hasColorDevice && !K4A_SUCCEEDED(k4a_device_get_color_mode_count(device, &color_mode_count)))
    {
        cout << "Failed to get color mode count" << endl;
        return K4A_RESULT_FAILED;
    }

    if (hasDepthDevice && !K4A_SUCCEEDED(k4a_device_get_depth_mode_count(device, &depth_mode_count)))
    {
        cout << "Failed to get depth mode count" << endl;
        return K4A_RESULT_FAILED;
    }

    if (!k4a_device_get_fps_mode_count(device, &fps_mode_count) == K4A_RESULT_SUCCEEDED)
    {
        cout << "Failed to get fps mode count" << endl;
        return K4A_RESULT_FAILED;
    }

    // 5. find the mode ids you want
    if (hasColorDevice && color_mode_count > 1)
    {
        for (uint32_t c = 1; c < color_mode_count; c++)
        {
            if (k4a_device_get_color_mode(device, c, &color_mode_info) == K4A_RESULT_SUCCEEDED)
            {
                if (color_mode_info.height >= 720)
                {
                    *color_mode_id = c;
                    break;
                }
            }
        }
    }

    if (hasDepthDevice && depth_mode_count > 1)
    {
        for (uint32_t d = 1; d < depth_mode_count; d++)
        {
            if (k4a_device_get_depth_mode(device, d, &depth_mode_info) == K4A_RESULT_SUCCEEDED)
            {
                if (depth_mode_info.height >= 576 && depth_mode_info.vertical_fov <= 65)
                {
                    *depth_mode_id = d;
                    break;
                }
            }
        }
    }

    if (fps_mode_count > 1)
    {
        uint32_t max_fps = 0;
        for (uint32_t f = 1; f < fps_mode_count; f++)
        {
            if (k4a_device_get_fps_mode(device, f, &fps_mode_info) == K4A_RESULT_SUCCEEDED)
            {
                if (fps_mode_info.fps >= (int)max_fps)
                {
                    max_fps = (uint32_t)fps_mode_info.fps;
                    *fps_mode_id = f;
                }
            }
        }
    }

    // 6. fps mode id must not be set to 0, which is Off, and either color mode id or depth mode id must not be set to 0
    if (*fps_mode_id == 0)
    {
        cout << "Fps mode id must not be set to 0 (Off)" << endl;
        return K4A_RESULT_FAILED;
    }

    if (*color_mode_id == 0 && *depth_mode_id == 0)
    {
        cout << "Either color mode id or depth mode id must not be set to 0 (Off)" << endl;
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

static int capture(std::string output_dir, uint8_t deviceId = K4A_DEVICE_DEFAULT)
{
    int returnCode = 1;
    k4a_device_t device = NULL;
    const int32_t TIMEOUT_IN_MS = 10000;
    k4a_transformation_t transformation = NULL;
    k4a_transformation_t transformation_color_downscaled = NULL;
    k4a_capture_t capture = NULL;
    std::string file_name = "";
    uint32_t device_count = 0;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    k4a_image_t color_image_downscaled = NULL;

    uint32_t color_mode_id = 0;
    uint32_t depth_mode_id = 0;
    uint32_t fps_mode_id = 0;

    device_count = k4a_device_get_installed_count();

    if (device_count == 0)
    {
        printf("No K4A devices found\n");
        return 0;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(deviceId, &device))
    {
        printf("Failed to open device\n");
        goto Exit;
    }

    if (!K4A_SUCCEEDED(get_device_mode_ids(device, &color_mode_id, &depth_mode_id, &fps_mode_id)))
    {
        cout << "Failed to get device mode ids" << endl;
        exit(-1);
    }

    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_mode_id = color_mode_id;
    config.depth_mode_id = depth_mode_id;
    config.fps_mode_id = fps_mode_id;
    config.synchronized_images_only = true; // ensures that depth and color images are both available in the capture

    k4a_calibration_t calibration;
    if (K4A_RESULT_SUCCEEDED !=
        k4a_device_get_calibration(device, config.depth_mode_id, config.color_mode_id, &calibration))
    {
        printf("Failed to get calibration\n");
        goto Exit;
    }

    transformation = k4a_transformation_create(&calibration);

    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
    {
        printf("Failed to start cameras\n");
        goto Exit;
    }

    // Get a capture
    switch (k4a_device_get_capture(device, &capture, TIMEOUT_IN_MS))
    {
    case K4A_WAIT_RESULT_SUCCEEDED:
        break;
    case K4A_WAIT_RESULT_TIMEOUT:
        printf("Timed out waiting for a capture\n");
        goto Exit;
    case K4A_WAIT_RESULT_FAILED:
    case K4A_WAIT_RESULT_UNSUPPORTED:
        printf("Failed to read a capture\n");
        goto Exit;
    }

    // Get a depth image
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        goto Exit;
    }

    // Get a color image
    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture\n");
        goto Exit;
    }

    // Compute color point cloud by warping color image into depth camera geometry
#ifdef _WIN32
    file_name = output_dir + "\\color_to_depth.ply";
#else
    file_name = output_dir + "/color_to_depth.ply";
#endif
    if (point_cloud_color_to_depth(transformation, depth_image, color_image, file_name.c_str()) == false)
    {
        goto Exit;
    }

    // Compute color point cloud by warping depth image into color camera geometry
#ifdef _WIN32
    file_name = output_dir + "\\depth_to_color.ply";
#else
    file_name = output_dir + "/depth_to_color.ply";
#endif
    if (point_cloud_depth_to_color(transformation, depth_image, color_image, file_name.c_str()) == false)
    {
        goto Exit;
    }

    // Compute color point cloud by warping depth image into color camera geometry with downscaled color image and
    // downscaled calibration. This example's goal is to show how to configure the calibration and use the
    // transformation API as it is when the user does not need a point cloud from high resolution transformed depth
    // image. The downscaling method here is naively to average binning 2x2 pixels, user should choose their own
    // appropriate downscale method on the color image, this example is only demonstrating the idea. However, no matter
    // what scale you choose to downscale the color image, please keep the aspect ratio unchanged (to ensure the
    // distortion parameters from original calibration can still be used for the downscaled image).
    k4a_calibration_t calibration_color_downscaled;
    memcpy(&calibration_color_downscaled, &calibration, sizeof(k4a_calibration_t));
    calibration_color_downscaled.color_camera_calibration.resolution_width /= 2;
    calibration_color_downscaled.color_camera_calibration.resolution_height /= 2;
    calibration_color_downscaled.color_camera_calibration.intrinsics.parameters.param.cx /= 2;
    calibration_color_downscaled.color_camera_calibration.intrinsics.parameters.param.cy /= 2;
    calibration_color_downscaled.color_camera_calibration.intrinsics.parameters.param.fx /= 2;
    calibration_color_downscaled.color_camera_calibration.intrinsics.parameters.param.fy /= 2;
    transformation_color_downscaled = k4a_transformation_create(&calibration_color_downscaled);
    color_image_downscaled = downscale_image_2x2_binning(color_image);
    if (color_image_downscaled == 0)
    {
        printf("Failed to downscaled color image\n");
        goto Exit;
    }

#ifdef _WIN32
    file_name = output_dir + "\\depth_to_color_downscaled.ply";
#else
    file_name = output_dir + "/depth_to_color_downscaled.ply";
#endif
    if (point_cloud_depth_to_color(transformation_color_downscaled,
                                   depth_image,
                                   color_image_downscaled,
                                   file_name.c_str()) == false)
    {
        goto Exit;
    }

    returnCode = 0;

Exit:
    if (depth_image != NULL)
    {
        k4a_image_release(depth_image);
    }
    if (color_image != NULL)
    {
        k4a_image_release(color_image);
    }
    if (capture != NULL)
    {
        k4a_capture_release(capture);
    }
    if (transformation != NULL)
    {
        k4a_transformation_destroy(transformation);
    }
    if (transformation_color_downscaled != NULL)
    {
        k4a_transformation_destroy(transformation_color_downscaled);
    }
    if (device != NULL)
    {
        k4a_device_close(device);
    }
    return returnCode;
}

// Timestamp in milliseconds. Defaults to 1 sec as the first couple frames don't contain color
static int playback(char *input_path, int timestamp = 1000, std::string output_filename = "output.ply")
{
    int returnCode = 1;
    k4a_playback_t playback = NULL;
    k4a_calibration_t calibration;
    k4a_transformation_t transformation = NULL;
    k4a_capture_t capture = NULL;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    k4a_image_t uncompressed_color_image = NULL;

    k4a_result_t result;
    k4a_stream_result_t stream_result;

    // Open recording
    result = k4a_playback_open(input_path, &playback);
    if (result != K4A_RESULT_SUCCEEDED || playback == NULL)
    {
        printf("Failed to open recording %s\n", input_path);
        goto Exit;
    }

    result = k4a_playback_seek_timestamp(playback, timestamp * 1000, K4A_PLAYBACK_SEEK_BEGIN);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to seek timestamp %d\n", timestamp);
        goto Exit;
    }
    printf("Seeking to timestamp: %d/%d (ms)\n",
           timestamp,
           (int)(k4a_playback_get_recording_length_usec(playback) / 1000));

    stream_result = k4a_playback_get_next_capture(playback, &capture);
    if (stream_result != K4A_STREAM_RESULT_SUCCEEDED || capture == NULL)
    {
        printf("Failed to fetch frame\n");
        goto Exit;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_playback_get_calibration(playback, &calibration))
    {
        printf("Failed to get calibration\n");
        goto Exit;
    }

    transformation = k4a_transformation_create(&calibration);

    // Fetch frame
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        goto Exit;
    }

    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0)
    {
        printf("Failed to get color image from capture\n");
        goto Exit;
    }

    ///////////////////////////////
    // Convert color frame from mjpeg to bgra
    k4a_image_format_t format;
    format = k4a_image_get_format(color_image);
    if (format != K4A_IMAGE_FORMAT_COLOR_MJPG)
    {
        printf("Color format not supported. Please use MJPEG\n");
        goto Exit;
    }

    int color_width, color_height;
    color_width = k4a_image_get_width_pixels(color_image);
    color_height = k4a_image_get_height_pixels(color_image);

    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                 color_width,
                                                 color_height,
                                                 color_width * 4 * (int)sizeof(uint8_t),
                                                 &uncompressed_color_image))
    {
        printf("Failed to create image buffer\n");
        goto Exit;
    }

    tjhandle tjHandle;
    tjHandle = tjInitDecompress();
    if (tjDecompress2(tjHandle,
                      k4a_image_get_buffer(color_image),
                      static_cast<unsigned long>(k4a_image_get_size(color_image)),
                      k4a_image_get_buffer(uncompressed_color_image),
                      color_width,
                      0, // pitch
                      color_height,
                      TJPF_BGRA,
                      TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0)
    {
        printf("Failed to decompress color frame\n");
        if (tjDestroy(tjHandle))
        {
            printf("Failed to destroy turboJPEG handle\n");
        }
        goto Exit;
    }
    if (tjDestroy(tjHandle))
    {
        printf("Failed to destroy turboJPEG handle\n");
    }
    ///////////////////////////////

    // Compute color point cloud by warping depth image into color camera geometry
    if (point_cloud_depth_to_color(transformation, depth_image, uncompressed_color_image, output_filename) == false)
    {
        printf("Failed to transform depth to color\n");
        goto Exit;
    }

    returnCode = 0;

Exit:
    if (playback != NULL)
    {
        k4a_playback_close(playback);
    }
    if (depth_image != NULL)
    {
        k4a_image_release(depth_image);
    }
    if (color_image != NULL)
    {
        k4a_image_release(color_image);
    }
    if (uncompressed_color_image != NULL)
    {
        k4a_image_release(uncompressed_color_image);
    }
    if (capture != NULL)
    {
        k4a_capture_release(capture);
    }
    if (transformation != NULL)
    {
        k4a_transformation_destroy(transformation);
    }
    return returnCode;
}

static void print_usage()
{
    printf("Usage: transformation_example capture <output_directory> [device_id]\n");
    printf("Usage: transformation_example playback <filename.mkv> [timestamp (ms)] [output_file]\n");
}

int main(int argc, char **argv)
{
    int returnCode = 0;

    if (argc < 2)
    {
        print_usage();
    }
    else
    {
        std::string mode = std::string(argv[1]);
        if (mode == "capture")
        {
            if (argc == 3)
            {
                returnCode = capture(argv[2]);
            }
            else if (argc == 4)
            {
                returnCode = capture(argv[2], (uint8_t)atoi(argv[3]));
            }
            else
            {
                print_usage();
            }
        }
        else if (mode == "playback")
        {
            if (argc == 3)
            {
                returnCode = playback(argv[2]);
            }
            else if (argc == 4)
            {
                returnCode = playback(argv[2], atoi(argv[3]));
            }
            else if (argc == 5)
            {
                returnCode = playback(argv[2], atoi(argv[3]), argv[4]);
            }
            else
            {
                print_usage();
            }
        }
        else
        {
            print_usage();
        }
    }

    return returnCode;
}