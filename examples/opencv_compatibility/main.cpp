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

int main(int argc, char ** /*argv*/)
{
    uint32_t device_count = 0;
    k4a_device_t device = NULL;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

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

    config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    k4a_calibration_t calibration;
    if (K4A_RESULT_SUCCEEDED !=
        k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration))
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
