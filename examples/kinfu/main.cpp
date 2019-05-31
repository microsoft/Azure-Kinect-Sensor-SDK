// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <k4a/k4a.h>

using namespace std;

// Enable HAVE_OPENCV macro after you installed opencv and opencv contrib modules (kinfu, viz), please refer to
// README.md
//#define HAVE_OPENCV
#ifdef HAVE_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/rgbd.hpp>
#include <opencv2/viz.hpp>
using namespace cv;
#endif

#ifdef HAVE_OPENCV
void initialize_kinfu_params(kinfu::Params &params,
                             const int width,
                             const int height,
                             const float fx,
                             const float fy,
                             const float cx,
                             const float cy)
{
    const Matx33f camera_matrix = Matx33f(fx, 0.0f, cx, 0.0f, fy, cy, 0.0f, 0.0f, 1.0f);
    params.frameSize = Size(width, height);
    params.intr = camera_matrix;
    params.depthFactor = 1000.0f;
}

template<typename T> Mat create_mat_from_buffer(T *data, int width, int height, int channels = 1)
{
    Mat mat(height, width, CV_MAKETYPE(DataType<T>::type, channels));
    memcpy(mat.data, data, width * height * channels * sizeof(T));
    return mat;
}
#endif

int main(int argc, char ** /*argv*/)
{
    k4a_device_t device = NULL;

    if (argc != 1)
    {
        printf("Usage: kinfu_example.exe\n");
        printf("Keys:   q - Quit\n");
        printf("        r - Reset KinFu\n");
        printf("        v - Enable Viz Render Cloud (default is OFF, enable it slows down frame rate)\n");
        return 2;
    }

    uint32_t device_count = k4a_device_get_installed_count();

    if (device_count == 0)
    {
        printf("No K4A devices found\n");
        return 1;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &device))
    {
        printf("Failed to open device\n");
        k4a_device_close(device);
        return 1;
    }

    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    // Retrive calibration
    k4a_calibration_t calibration;
    if (K4A_RESULT_SUCCEEDED !=
        k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration))
    {
        printf("Failed to get calibration\n");
        k4a_device_close(device);
        return 1;
    }

    // Start cameras
    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
    {
        printf("Failed to start device\n");
        k4a_device_close(device);
        return 1;
    }

#ifdef HAVE_OPENCV
    setUseOptimized(true);

    // Retrieve calibration parameters
    k4a_calibration_intrinsic_parameters_t *intrinsics = &calibration.depth_camera_calibration.intrinsics.parameters;
    const int width = calibration.depth_camera_calibration.resolution_width;
    const int height = calibration.depth_camera_calibration.resolution_height;

    // Initialize kinfu parameters
    Ptr<kinfu::Params> params;
    params = kinfu::Params::defaultParams();
    initialize_kinfu_params(
        *params, width, height, intrinsics->param.fx, intrinsics->param.fy, intrinsics->param.cx, intrinsics->param.cy);

    // Distortion coefficients
    Matx<float, 1, 8> distCoeffs;
    distCoeffs(0) = intrinsics->param.k1;
    distCoeffs(1) = intrinsics->param.k2;
    distCoeffs(2) = intrinsics->param.p1;
    distCoeffs(3) = intrinsics->param.p2;
    distCoeffs(4) = intrinsics->param.k3;
    distCoeffs(5) = intrinsics->param.k4;
    distCoeffs(6) = intrinsics->param.k5;
    distCoeffs(7) = intrinsics->param.k6;

    // Initialize undistort maps
    UMat map1, map2;
    initUndistortRectifyMap(params->intr, distCoeffs, noArray(), params->intr, params->frameSize, CV_16SC2, map1, map2);

    // Create KinectFusion module instance
    Ptr<kinfu::KinFu> kf;
    kf = kinfu::KinFu::create(params);
    namedWindow("AzureKinect KinectFusion Example");
    viz::Viz3d visualization("AzureKinect KinectFusion Example");

    bool stop = false;
    bool renderViz = false;
    k4a_capture_t capture = NULL;
    k4a_image_t depth_image = NULL;
    const int32_t TIMEOUT_IN_MS = 1000;
    while (!stop && !visualization.wasStopped())
    {
        // Get a depth frame
        switch (k4a_device_get_capture(device, &capture, TIMEOUT_IN_MS))
        {
        case K4A_WAIT_RESULT_SUCCEEDED:
            break;
        case K4A_WAIT_RESULT_TIMEOUT:
            printf("Timed out waiting for a capture\n");
            continue;
            break;
        case K4A_WAIT_RESULT_FAILED:
            printf("Failed to read a capture\n");
            k4a_device_close(device);
            return 1;
        }

        // Retrieve depth image
        depth_image = k4a_capture_get_depth_image(capture);
        if (depth_image == NULL)
        {
            printf("Depth16 None\n");
            k4a_capture_release(capture);
            continue;
        }

        // Create frame from depth buffer
        uint8_t *buffer = k4a_image_get_buffer(depth_image);
        uint16_t *depth_buffer = reinterpret_cast<uint16_t *>(buffer);
        UMat frame;
        create_mat_from_buffer<uint16_t>(depth_buffer, width, height).copyTo(frame);

        // Undistort the depth frame (bilinear approach will introduce floating noise between valid and invalid depth,
        // as an example, we use nearest neighbor here)
        UMat undistortedFrame;
        remap(frame, undistortedFrame, map1, map2, INTER_NEAREST);
        if (undistortedFrame.empty())
        {
            k4a_image_release(depth_image);
            k4a_capture_release(capture);
            continue;
        }

        // Update KinectFusion
        if (!kf->update(undistortedFrame))
        {
            printf("Reset KinectFusion\n");
            kf->reset();
            k4a_image_release(depth_image);
            k4a_capture_release(capture);
            continue;
        }

        // Retrieve rendered TSDF
        UMat tsdfRender;
        kf->render(tsdfRender);

        // Retrieve fused point cloud and normals
        UMat points;
        UMat normals;
        kf->getCloud(points, normals);

        // Show TSDF rendering
        imshow("AzureKinect KinectFusion Example", tsdfRender);

        // Show fused point cloud and normals
        if (!points.empty() && !normals.empty() && renderViz)
        {
            viz::WCloud cloud(points, viz::Color::white());
            viz::WCloudNormals cloudNormals(points, normals, 1, 0.01, viz::Color::cyan());
            visualization.showWidget("cloud", cloud);
            visualization.showWidget("normals", cloudNormals);
            visualization.showWidget("worldAxes", viz::WCoordinateSystem());
            Vec3d volSize = kf->getParams().voxelSize * kf->getParams().volumeDims;
            visualization.showWidget("cube", viz::WCube(Vec3d::all(0), volSize), kf->getParams().volumePose);
            visualization.spinOnce(1, true);
        }

        // Key controls
        const int32_t key = waitKey(5);
        if (key == 'r')
        {
            printf("Reset KinectFusion\n");
            kf->reset();
        }
        else if (key == 'v')
        {
            renderViz = true;
        }
        else if (key == 'q')
        {
            stop = true;

            // Output the fused point cloud from KinectFusion
            Mat out_points;
            points.copyTo(out_points);

            printf("Saving fused point cloud into ply file ...\n");

            // Save to the ply file
#define PLY_START_HEADER "ply"
#define PLY_END_HEADER "end_header"
#define PLY_ASCII "format ascii 1.0"
#define PLY_ELEMENT_VERTEX "element vertex"
            string output_file_name = "kinectfusion_output.ply";
            ofstream ofs(output_file_name); // text mode first
            ofs << PLY_START_HEADER << endl;
            ofs << PLY_ASCII << endl;
            ofs << PLY_ELEMENT_VERTEX << " " << out_points.rows << endl;
            ofs << "property float x" << endl;
            ofs << "property float y" << endl;
            ofs << "property float z" << endl;
            ofs << "property uchar red" << endl;
            ofs << "property uchar green" << endl;
            ofs << "property uchar blue" << endl;
            ofs << PLY_END_HEADER << endl;
            ofs.close();

            stringstream ss;
            for (int i = 0; i < out_points.rows; ++i)
            {
                // image data is BGR
                ss << out_points.at<float>(i, 0) << " " << out_points.at<float>(i, 1) << " "
                   << out_points.at<float>(i, 2);
                ss << " " << 255 << " " << 255 << " " << 255;
                ss << endl;
            }
            ofstream ofs_text(output_file_name, ios::out | ios::app);
            ofs_text.write(ss.str().c_str(), (streamsize)ss.str().length());
        }

        k4a_image_release(depth_image);
        k4a_capture_release(capture);
    }

    destroyAllWindows();
#endif

    k4a_device_close(device);

    return 0;
}