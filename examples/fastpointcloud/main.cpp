// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <math.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

static void create_xy_table(const k4a_calibration_t *calibration, k4a_image_t xy_table)
{
    k4a_float2_t *table_data = (k4a_float2_t *)(void *)k4a_image_get_buffer(xy_table);

    int width = calibration->depth_camera_calibration.resolution_width;
    int height = calibration->depth_camera_calibration.resolution_height;

    k4a_float2_t p;
    k4a_float3_t ray;
    int valid;

    for (int y = 0, idx = 0; y < height; y++)
    {
        p.xy.y = (float)y;
        for (int x = 0; x < width; x++, idx++)
        {
            p.xy.x = (float)x;

            k4a_calibration_2d_to_3d(
                calibration, &p, 1.f, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &ray, &valid);

            if (valid)
            {
                table_data[idx].xy.x = ray.xyz.x;
                table_data[idx].xy.y = ray.xyz.y;
            }
            else
            {
                table_data[idx].xy.x = nanf("");
                table_data[idx].xy.y = nanf("");
            }
        }
    }
}

static void generate_point_cloud(const k4a_image_t depth_image,
                                 const k4a_image_t xy_table,
                                 k4a_image_t point_cloud,
                                 int *point_count)
{
    int width = k4a_image_get_width_pixels(depth_image);
    int height = k4a_image_get_height_pixels(depth_image);

    uint16_t *depth_data = (uint16_t *)(void *)k4a_image_get_buffer(depth_image);
    k4a_float2_t *xy_table_data = (k4a_float2_t *)(void *)k4a_image_get_buffer(xy_table);
    k4a_float3_t *point_cloud_data = (k4a_float3_t *)(void *)k4a_image_get_buffer(point_cloud);

    *point_count = 0;
    for (int i = 0; i < width * height; i++)
    {
        if (depth_data[i] != 0 && !isnan(xy_table_data[i].xy.x) && !isnan(xy_table_data[i].xy.y))
        {
            point_cloud_data[i].xyz.x = xy_table_data[i].xy.x * (float)depth_data[i];
            point_cloud_data[i].xyz.y = xy_table_data[i].xy.y * (float)depth_data[i];
            point_cloud_data[i].xyz.z = (float)depth_data[i];
            (*point_count)++;
        }
        else
        {
            point_cloud_data[i].xyz.x = nanf("");
            point_cloud_data[i].xyz.y = nanf("");
            point_cloud_data[i].xyz.z = nanf("");
        }
    }
}

static void write_point_cloud(const char *file_name, const k4a_image_t point_cloud, int point_count)
{
    int width = k4a_image_get_width_pixels(point_cloud);
    int height = k4a_image_get_height_pixels(point_cloud);

    k4a_float3_t *point_cloud_data = (k4a_float3_t *)(void *)k4a_image_get_buffer(point_cloud);

    // save to the ply file
    std::ofstream ofs(file_name); // text mode first
    ofs << "ply" << std::endl;
    ofs << "format ascii 1.0" << std::endl;
    ofs << "element vertex"
        << " " << point_count << std::endl;
    ofs << "property float x" << std::endl;
    ofs << "property float y" << std::endl;
    ofs << "property float z" << std::endl;
    ofs << "end_header" << std::endl;
    ofs.close();

    std::stringstream ss;
    for (int i = 0; i < width * height; i++)
    {
        if (isnan(point_cloud_data[i].xyz.x) || isnan(point_cloud_data[i].xyz.y) || isnan(point_cloud_data[i].xyz.z))
        {
            continue;
        }

        ss << (float)point_cloud_data[i].xyz.x << " " << (float)point_cloud_data[i].xyz.y << " "
           << (float)point_cloud_data[i].xyz.z << std::endl;
    }

    std::ofstream ofs_text(file_name, std::ios::out | std::ios::app);
    ofs_text.write(ss.str().c_str(), (std::streamsize)ss.str().length());
}

static k4a_result_t get_device_mode_ids(k4a_device_t device, uint32_t *depth_mode_id, uint32_t *fps_mode_id)
{
    // 1. declare device info and depth and color modes
    k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, 0 };
    k4a_device_info_t device_info = { sizeof(k4a_device_info_t), K4A_ABI_VERSION, 0 };

    // 2. get the count of modes
    uint32_t depth_mode_count = 0;
    uint32_t fps_mode_count = 0;

    // 3. get device info
    if (!k4a_device_get_info(device, &device_info) == K4A_RESULT_SUCCEEDED)
    {
        cout << "Failed to get device info" << endl;
        return K4A_RESULT_FAILED;
    }

    bool hasDepthDevice = (device_info.capabilities.bitmap.bHasDepth == 1);

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

    // 4. find the mode ids you want
    if (hasDepthDevice && depth_mode_count > 1)
    {
        for (uint32_t d = 1; d < depth_mode_count; d++)
        {
            k4a_depth_mode_info_t depth_mode = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, 0 };
            if (k4a_device_get_depth_mode(device, d, &depth_mode) == K4A_RESULT_SUCCEEDED)
            {
                if (depth_mode.height >= 512 && depth_mode.vertical_fov >= 120)
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

    // 5. fps mode id must not be set to 0, which is Off, and either color mode id or depth mode id must not be set to 0
    if (*fps_mode_id == 0)
    {
        cout << "Fps mode id must not be set to 0 (Off)" << endl;
        return K4A_RESULT_FAILED;
    }

    if (*depth_mode_id == 0)
    {
        cout << "Either color mode id or depth mode id must not be set to 0 (Off)" << endl;
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

int main(int argc, char **argv)
{
    int returnCode = 1;
    k4a_device_t device = NULL;
    const int32_t TIMEOUT_IN_MS = 1000;
    k4a_capture_t capture = NULL;
    std::string file_name;
    uint32_t device_count = 0;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_image_t depth_image = NULL;
    k4a_image_t xy_table = NULL;
    k4a_image_t point_cloud = NULL;
    int point_count = 0;

    uint32_t depth_mode_id = 0;
    uint32_t fps_mode_id = 0;

    if (argc != 2)
    {
        printf("fastpointcloud.exe <output file>\n");
        returnCode = 2;
        goto Exit;
    }

    file_name = argv[1];

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

    if (!K4A_SUCCEEDED(get_device_mode_ids(device, &depth_mode_id, &fps_mode_id)))
    {
        cout << "Failed to get device mode ids" << endl;
        exit(-1);
    }

    config.depth_mode_id = depth_mode_id;
    config.fps_mode_id = fps_mode_id;

    k4a_calibration_t calibration;
    if (K4A_RESULT_SUCCEEDED !=
        k4a_device_get_calibration(device, config.depth_mode_id, config.color_mode_id, &calibration))
    {
        printf("Failed to get calibration\n");
        goto Exit;
    }

    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
                     calibration.depth_camera_calibration.resolution_width,
                     calibration.depth_camera_calibration.resolution_height,
                     calibration.depth_camera_calibration.resolution_width * (int)sizeof(k4a_float2_t),
                     &xy_table);

    create_xy_table(&calibration, xy_table);

    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
                     calibration.depth_camera_calibration.resolution_width,
                     calibration.depth_camera_calibration.resolution_height,
                     calibration.depth_camera_calibration.resolution_width * (int)sizeof(k4a_float3_t),
                     &point_cloud);

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

    generate_point_cloud(depth_image, xy_table, point_cloud, &point_count);

    write_point_cloud(file_name.c_str(), point_cloud, point_count);

    k4a_image_release(depth_image);
    k4a_capture_release(capture);
    k4a_image_release(xy_table);
    k4a_image_release(point_cloud);

    returnCode = 0;
Exit:
    if (device != NULL)
    {
        k4a_device_close(device);
    }

    return returnCode;
}
