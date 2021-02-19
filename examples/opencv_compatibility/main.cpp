// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <stdio.h>
#include <vector>
using namespace std;

#include "opencv2/core.hpp"
#include "opencv2/calib3d.hpp"
using namespace cv;

static void clean_up(k4a_device_t device)
{
    if (device != NULL)
    {
        k4a_device_close(device);
    }
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
        printf("Failed to get device info\n");
        return K4A_RESULT_FAILED;
    }

    hasDepthDevice = (device_info.capabilities.bitmap.bHasDepth == 1);
    hasColorDevice = (device_info.capabilities.bitmap.bHasColor == 1);

    if (hasColorDevice && !K4A_SUCCEEDED(k4a_device_get_color_mode_count(device, &color_mode_count)))
    {
        printf("Failed to get color mode count\n");
        return K4A_RESULT_FAILED;
    }

    if (hasDepthDevice && !K4A_SUCCEEDED(k4a_device_get_depth_mode_count(device, &depth_mode_count)))
    {
        printf("Failed to get depth mode count\n");
        return K4A_RESULT_FAILED;
    }

    if (!k4a_device_get_fps_mode_count(device, &fps_mode_count) == K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to get fps mode count\n");
        return K4A_RESULT_FAILED;
    }

    // 5. find the mode ids you want
    if (hasColorDevice && color_mode_count > 1)
    {
        for (uint32_t c = 1; c < color_mode_count; c++)
        {
            if (k4a_device_get_color_mode(device, c, &color_mode_info) == K4A_RESULT_SUCCEEDED)
            {
                if (color_mode_info.height >= 1080)
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
                if (depth_mode_info.height <= 512 && depth_mode_info.vertical_fov <= 65)
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
        printf("Fps mode id must not be set to 0 (Off)\n");
        return K4A_RESULT_FAILED;
    }

    if (*color_mode_id == 0 && *depth_mode_id == 0)
    {
        printf("Either color mode id or depth mode id must not be set to 0 (Off)\n");
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

int main(int argc, char ** /*argv*/)
{
    uint32_t device_count = 0;
    k4a_device_t device = NULL;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    uint32_t color_mode_id = 0;
    uint32_t depth_mode_id = 0;
    uint32_t fps_mode_id = 0;

    if (argc != 1)
    {
        printf("Usage: opencv_example.exe\n");
        return 2;
    }

    device_count = k4a_device_get_installed_count();

    if (device_count == 0)
    {
        printf("No K4A devices found\n");
        return 1;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &device))
    {
        printf("Failed to open device\n");
        clean_up(device);
        return 1;
    }

    if (!K4A_SUCCEEDED(get_device_mode_ids(device, &color_mode_id, &depth_mode_id, &fps_mode_id)))
    {
        cout << "Failed to get device mode ids" << endl;
        exit(-1);
    }

    config.color_mode_id = color_mode_id;
    config.depth_mode_id = depth_mode_id;
    config.fps_mode_id = fps_mode_id;

    k4a_calibration_t calibration;
    if (K4A_RESULT_SUCCEEDED !=
        k4a_device_get_calibration(device, config.depth_mode_id, config.color_mode_id, &calibration))
    {
        printf("Failed to get calibration\n");
        clean_up(device);
        return 1;
    }

    vector<k4a_float3_t> points_3d = { { { 0.f, 0.f, 1000.f } },          // color camera center
                                       { { -1000.f, -1000.f, 1000.f } },  // color camera top left
                                       { { 1000.f, -1000.f, 1000.f } },   // color camera top right
                                       { { 1000.f, 1000.f, 1000.f } },    // color camera bottom right
                                       { { -1000.f, 1000.f, 1000.f } } }; // color camera bottom left

    // k4a project function
    vector<k4a_float2_t> k4a_points_2d(points_3d.size());
    for (size_t i = 0; i < points_3d.size(); i++)
    {
        int valid = 0;
        k4a_calibration_3d_to_2d(&calibration,
                                 &points_3d[i],
                                 K4A_CALIBRATION_TYPE_COLOR,
                                 K4A_CALIBRATION_TYPE_DEPTH,
                                 &k4a_points_2d[i],
                                 &valid);
    }

    // converting the calibration data to OpenCV format
    // extrinsic transformation from color to depth camera
    Mat se3 =
        Mat(3, 3, CV_32FC1, calibration.extrinsics[K4A_CALIBRATION_TYPE_COLOR][K4A_CALIBRATION_TYPE_DEPTH].rotation);
    Mat r_vec = Mat(3, 1, CV_32FC1);
    Rodrigues(se3, r_vec);
    Mat t_vec =
        Mat(3, 1, CV_32F, calibration.extrinsics[K4A_CALIBRATION_TYPE_COLOR][K4A_CALIBRATION_TYPE_DEPTH].translation);

    // intrinsic parameters of the depth camera
    k4a_calibration_intrinsic_parameters_t *intrinsics = &calibration.depth_camera_calibration.intrinsics.parameters;
    vector<float> _camera_matrix = {
        intrinsics->param.fx, 0.f, intrinsics->param.cx, 0.f, intrinsics->param.fy, intrinsics->param.cy, 0.f, 0.f, 1.f
    };
    Mat camera_matrix = Mat(3, 3, CV_32F, &_camera_matrix[0]);
    vector<float> _dist_coeffs = { intrinsics->param.k1, intrinsics->param.k2, intrinsics->param.p1,
                                   intrinsics->param.p2, intrinsics->param.k3, intrinsics->param.k4,
                                   intrinsics->param.k5, intrinsics->param.k6 };
    Mat dist_coeffs = Mat(8, 1, CV_32F, &_dist_coeffs[0]);

    // OpenCV project function
    vector<Point2f> cv_points_2d(points_3d.size());
    projectPoints(*reinterpret_cast<vector<Point3f> *>(&points_3d),
                  r_vec,
                  t_vec,
                  camera_matrix,
                  dist_coeffs,
                  cv_points_2d);

    for (size_t i = 0; i < points_3d.size(); i++)
    {
        printf("3d point:\t\t\t(%.5f, %.5f, %.5f)\n", points_3d[i].v[0], points_3d[i].v[1], points_3d[i].v[2]);
        printf("OpenCV projectPoints:\t\t(%.5f, %.5f)\n", cv_points_2d[i].x, cv_points_2d[i].y);
        printf("k4a_calibration_3d_to_2d:\t(%.5f, %.5f)\n\n", k4a_points_2d[i].v[0], k4a_points_2d[i].v[1]);
    }

    clean_up(device);
    return 0;
}