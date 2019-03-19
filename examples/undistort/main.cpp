// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>

#define INVALID INT32_MIN

typedef struct _pinhole_t
{
    float px;
    float py;
    float fx;
    float fy;

    int width;
    int height;
} pinhole_t;

typedef struct _coordinate_t
{
    int x;
    int y;
} coordinate_t;

static pinhole_t create_pinhole(float field_of_view, int width, int height)
{
    pinhole_t pinhole;

    float f = 0.5f / tanf(0.5f * field_of_view * 3.14159265f / 180.f);

    pinhole.px = (float)width / 2.f;
    pinhole.py = (float)height / 2.f;
    pinhole.fx = f * (float)width;
    pinhole.fy = f * (float)height;
    pinhole.width = width;
    pinhole.height = height;

    return pinhole;
}

static void create_undistortion_lut(const k4a_calibration_t *calibration,
                                    const k4a_calibration_type_t camera,
                                    const pinhole_t *pinhole,
                                    k4a_image_t lut)
{
    coordinate_t *lut_data = (coordinate_t *)(void *)k4a_image_get_buffer(lut);

    k4a_float3_t ray;
    ray.xyz.z = 1.f;

    int src_width = calibration->depth_camera_calibration.resolution_width;
    int src_height = calibration->depth_camera_calibration.resolution_height;
    if (camera == K4A_CALIBRATION_TYPE_COLOR)
    {
        src_width = calibration->color_camera_calibration.resolution_width;
        src_height = calibration->color_camera_calibration.resolution_height;
    }

    for (int y = 0, idx = 0; y < pinhole->height; y++)
    {
        ray.xyz.y = ((float)y - pinhole->py) / pinhole->fy;

        for (int x = 0; x < pinhole->width; x++, idx++)
        {
            ray.xyz.x = ((float)x - pinhole->px) / pinhole->fx;

            k4a_float2_t distorted;
            int valid;
            k4a_calibration_3d_to_2d(calibration, &ray, camera, camera, &distorted, &valid);

            coordinate_t src;
            // Remapping via nearest neighbor interpolation
            src.x = (int)floorf(distorted.xy.x + 0.5f);
            src.y = (int)floorf(distorted.xy.y + 0.5f);

            if (valid && src.x >= 0 && src.x < src_width && src.y >= 0 && src.y < src_height)
            {
                lut_data[idx] = src;
            }
            else
            {
                lut_data[idx].x = INVALID;
                lut_data[idx].y = INVALID;
            }
        }
    }
}

static void remap(const k4a_image_t src, const k4a_image_t lut, k4a_image_t dst)
{
    int src_width = k4a_image_get_width_pixels(src);
    int dst_width = k4a_image_get_width_pixels(dst);
    int dst_height = k4a_image_get_height_pixels(dst);

    uint16_t *src_data = (uint16_t *)(void *)k4a_image_get_buffer(src);
    uint16_t *dst_data = (uint16_t *)(void *)k4a_image_get_buffer(dst);
    coordinate_t *lut_data = (coordinate_t *)(void *)k4a_image_get_buffer(lut);

    memset(dst_data, 0, (size_t)dst_width * (size_t)dst_height * sizeof(uint16_t));

    for (int i = 0; i < dst_width * dst_height; i++)
    {
        if (lut_data[i].x != INVALID && lut_data[i].y != INVALID)
        {
            dst_data[i] = src_data[lut_data[i].y * src_width + lut_data[i].x];
        }
    }
}

static void write_csv_file(const char *file_name, const k4a_image_t src)
{
    FILE *fp = 0;
#ifdef _MSC_VER
    fopen_s(&fp, file_name, "w");
#else
    fp = fopen(file_name, "w");
#endif

    int width = k4a_image_get_width_pixels(src);
    int height = k4a_image_get_height_pixels(src);
    uint16_t *src_data = (uint16_t *)(void *)k4a_image_get_buffer(src);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            fprintf(fp, "%d", src_data[y * width + x]);
            if (x < width - 1)
            {
                fprintf(fp, ",");
            }
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
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
    k4a_image_t lut = NULL;
    k4a_image_t undistorted = NULL;

    // Generate a pinhole model with 120 degree field of view and a resolution of 1024x1024 pixels
    pinhole_t pinhole = create_pinhole(120, 1024, 1024);

    if (argc != 2)
    {
        printf("undistort.exe <output file>\n");
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

    config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    k4a_calibration_t calibration;
    if (K4A_RESULT_SUCCEEDED !=
        k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration))
    {
        printf("Failed to get calibration\n");
        goto Exit;
    }

    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
                     pinhole.width,
                     pinhole.height,
                     pinhole.width * (int)sizeof(coordinate_t),
                     &lut);

    create_undistortion_lut(&calibration, K4A_CALIBRATION_TYPE_DEPTH, &pinhole, lut);

    k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
                     pinhole.width,
                     pinhole.height,
                     pinhole.width * (int)sizeof(uint16_t),
                     &undistorted);

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

    remap(depth_image, lut, undistorted);

    write_csv_file(file_name.c_str(), undistorted);

    k4a_image_release(depth_image);
    k4a_capture_release(capture);
    k4a_image_release(lut);
    k4a_image_release(undistorted);

    returnCode = 0;
Exit:
    if (device != NULL)
    {
        k4a_device_close(device);
    }

    return returnCode;
}
