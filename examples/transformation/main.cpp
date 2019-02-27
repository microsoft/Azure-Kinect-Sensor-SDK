// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <string>
#include "transformation_helpers.h"

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

int main(int argc, char **argv)
{
    int returnCode = 1;
    k4a_device_t device = NULL;
    const int32_t TIMEOUT_IN_MS = 1000;
    k4a_transformation_t transformation = NULL;
    k4a_capture_t capture = NULL;
    std::string output_dir = "";
    std::string file_name = "";
    uint32_t device_count = 0;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;

    if (argc != 2)
    {
        printf("transformation_example.exe <ouput directory>\n");
        returnCode = 2;
        goto Exit;
    }

    output_dir = argv[1];

    device_count = k4a_device_get_installed_count();

    if (device_count == 0)
    {
        printf("No K4A devices found\n");
        return 0;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &device))
    {
        printf("Failed to open device\n");
        goto Exit;
    }

    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.synchronized_images_only = true; // ensures that depth and color images are both available in the capture

    k4a_calibration_t calibration;
    if (K4A_RESULT_SUCCEEDED !=
        k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration))
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
    file_name = output_dir + "\\color_to_depth.ply";
    if (point_cloud_color_to_depth(transformation, depth_image, color_image, file_name.c_str()) == false)
    {
        goto Exit;
    }

    // Compute color point cloud by warping depth image into color camera geometry
    file_name = output_dir + "\\depth_to_color.ply";
    if (point_cloud_depth_to_color(transformation, depth_image, color_image, file_name.c_str()) == false)
    {
        goto Exit;
    }

    k4a_image_release(depth_image);
    k4a_image_release(color_image);
    k4a_capture_release(capture);

    k4a_transformation_destroy(transformation);

    returnCode = 0;
Exit:
    if (device != NULL)
    {
        k4a_device_stop_cameras(device);
        k4a_device_close(device);
    }

    return returnCode;
}
