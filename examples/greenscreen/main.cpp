#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <getopt.h>

#include <k4a/k4a.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using std::cout;
using std::endl;
using std::vector;

#include "transformation.h"
#include "MultiDeviceCapturer.h"

// Allowing at least 160 microseconds between depth cameras should ensure they do not interfere with one another.
constexpr uint32_t MIN_TIME_BETWEEN_DEPTH_CAMERA_PICTURES_USEC = 160;

// ideally, we could generalize this to many OpenCV types
cv::Mat k4a_color_to_opencv(const k4a::image &im)
{
    // TODO this only handles mjpg
    cv::Mat raw_data(1, im.get_size(), CV_8UC1, (void *)im.get_buffer());
    cv::Mat decoded = cv::imdecode(raw_data, cv::IMREAD_COLOR);
    return decoded;
}

cv::Mat k4a_depth_to_opencv(const k4a::image &im)
{
    return cv::Mat(im.get_height_pixels(),
                   im.get_width_pixels(),
                   CV_16U,
                   (void *)im.get_buffer(),
                   im.get_stride_bytes());
}

cv::Matx33f calibration_to_color_camera_matrix(const k4a::calibration &cal)
{
    const k4a_calibration_intrinsic_parameters_t::_param &i = cal.color_camera_calibration.intrinsics.parameters.param;
    cv::Matx33f camera_matrix = cv::Matx33f::zeros();
    camera_matrix(0, 0) = i.fx;
    camera_matrix(1, 1) = i.fx;
    camera_matrix(0, 2) = i.cx;
    camera_matrix(1, 2) = i.cy;
    camera_matrix(2, 2) = 1;
    return camera_matrix;
}

Transformation calibration_to_depth_to_color_transformation(const k4a::calibration &cal)
{
    const k4a_calibration_extrinsics_t &ex = cal.extrinsics[K4A_CALIBRATION_TYPE_DEPTH][K4A_CALIBRATION_TYPE_COLOR];
    Transformation tr;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            tr.R(i, j) = ex.rotation[i * 3 + j];
        }
    }
    tr.t = cv::Vec3d(ex.translation[0], ex.translation[1], ex.translation[2]);
    return tr;
}

void set_calibration_depth_to_color_from_transformation(k4a::calibration &cal, const Transformation &tr)
{
    k4a_calibration_extrinsics_t &ex = cal.extrinsics[K4A_CALIBRATION_TYPE_DEPTH][K4A_CALIBRATION_TYPE_COLOR];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            ex.rotation[i * 3 + j] = static_cast<float>(tr.R(i, j));
        }
    }
    for (int i = 0; i < 3; ++i)
    {
        ex.translation[i] = static_cast<float>(tr.t[i]);
    }
}

vector<float> calibration_to_color_camera_dist_coeffs(const k4a::calibration &cal)
{
    const k4a_calibration_intrinsic_parameters_t::_param &i = cal.color_camera_calibration.intrinsics.parameters.param;
    return { i.k1, i.k2, i.p1, i.p2, i.k3, i.k4, i.k5, i.k6 };
}

// Takes the images by value so we can change them when shown.
bool find_chessboard_corners_helper(cv::Mat master_color_image,
                                    cv::Mat sub_color_image,
                                    const cv::Size &chessboard_pattern,
                                    vector<cv::Point2f> &master_chessboard_corners,
                                    vector<cv::Point2f> &sub_chessboard_corners)
{
    bool found_chessboard_master = cv::findChessboardCorners(master_color_image,
                                                             chessboard_pattern,
                                                             master_chessboard_corners);
    bool found_chessboard_sub = cv::findChessboardCorners(sub_color_image, chessboard_pattern, sub_chessboard_corners);

    // Cover the failure cases where chessboards were not found in one or both images.
    if (!found_chessboard_master || !found_chessboard_sub)
    {
        if (found_chessboard_master)
        {
            cout << "Could not find the chessboard corners in the subordinate image. Trying again...\n";
        }
        // Likewise, if the chessboard was found in the subordinate image, it was not found in the master image.
        else if (found_chessboard_sub)
        {
            cout << "Could not find the chessboard corners in the master image. Trying again...\n";
        }
        // The only remaining case is the corners were in neither image.
        else
        {
            cout << "Could not find the chessboard corners in either image. Trying again...\n";
        }
        return false;
    }
    // Before we go on, there's a quick problem with calibration to address.  Because the chessboard looks the same when
    // rotated 180 degrees, it is possible that the chessboard corner finder may find the correct points, but in the
    // wrong order.

    // A visual:
    //        Image 1                  Image 2
    // .....................    .....................
    // .....................    .....................
    // .........xxxxx2......    .....xxxxx1..........
    // .........xxxxxx......    .....xxxxxx..........
    // .........xxxxxx......    .....xxxxxx..........
    // .........1xxxxx......    .....2xxxxx..........
    // .....................    .....................
    // .....................    .....................

    // The problem occurs when this case happens: the find_chessboard() function correctly identifies the points on the
    // checkerboard (shown as 'x's) but the order of those points differs between images taken by the two cameras.
    // Specifically, the first point in the list of points found for the first image (1) is the *last* point in the list
    // of points found for the second image (2), though they correspond to the same physical point on the chessboard.

    // To avoid this problem, we can make the assumption that both of the cameras will be oriented in a similar manner
    // (e.g. turning one of the cameras upside down will break this assumption) and enforce that the vector between the
    // first and last points found in pixel space (which will be at opposite ends of the chessboard) are pointing the
    // same direction- so, the dot product of the two vectors is positive.

    cv::Vec2f master_image_corners_vec = master_chessboard_corners.back() - master_chessboard_corners.front();
    cv::Vec2f sub_image_corners_vec = sub_chessboard_corners.back() - sub_chessboard_corners.front();
    if (master_image_corners_vec.dot(sub_image_corners_vec) <= 0.0)
    {
        std::reverse(sub_chessboard_corners.begin(), sub_chessboard_corners.end());
    }

    // Comment out this section to not show the calibration output
    cv::drawChessboardCorners(master_color_image, chessboard_pattern, master_chessboard_corners, true);
    cv::drawChessboardCorners(sub_color_image, chessboard_pattern, sub_chessboard_corners, true);
    cv::imshow("Chessboard view from master camera", master_color_image);
    cv::waitKey(500);
    cv::imshow("Chessboard view from subordinate camera", sub_color_image);
    cv::waitKey(500);

    return true;
}

Transformation stereo_calibration(const k4a::calibration &master_calib,
                                  const k4a::calibration &sub_calib,
                                  const vector<cv::Point2f> &master_chessboard_corners,
                                  const vector<cv::Point2f> &sub_chessboard_corners,
                                  const cv::Size &image_size,
                                  const cv::Size &chessboard_pattern,
                                  float chessboard_square_length)
{
    // We have points in each image that correspond to the corners that the findChessboardCorners function found.
    // However, we still need the points in 3 dimensions that these points correspond to. Because we are ultimately only
    // interested in find a transformation between two cameras, these points don't have to correspond to an external
    // "origin" point. The only important thing is that the relative distances between points are accurate. As a result,
    // we can simply make the first corresponding point (0, 0) and construct the remaining points based on that one. The
    // order of points inserted into the vector here matches the ordering of findChessboardCorners. The units of these
    // points are in millimeters, mostly because the depth provided by the depth cameras is also provided in
    // millimeters, which makes for easy comparison.
    vector<cv::Point3f> chessboard_corners_world;
    for (int h = 0; h < chessboard_pattern.height; ++h)
    {
        for (int w = 0; w < chessboard_pattern.width; ++w)
        {
            chessboard_corners_world.emplace_back(
                cv::Point3f{ w * chessboard_square_length, h * chessboard_square_length, 0.0 });
        }
    }

    // Calibrating the cameras requires a lot of data. OpenCV's stereoCalibrate function requires:
    // - a list of points in real 3d space that will be used to calibrate*
    // - a corresponding list of pixel coordinates as seen by the first camera*
    // - a corresponding list of pixel coordinates as seen by the second camera*
    // - the camera matrix of the first camera
    // - the distortion coefficients of the first camera
    // - the camera matrix of the second camera
    // - the distortion coefficients of the second camera
    // - the size (in pixels) of the images
    // - R: stereoCalibrate stores the rotation matrix from the first camera to the second here
    // - t: stereoCalibrate stores the translation vector from the first camera to the second here
    // - E: stereoCalibrate stores the essential matrix here (we don't use this)
    // - F: stereoCalibrate stores the fundamental matrix here (we don't use this)
    //
    // * note: OpenCV's stereoCalibrate actually requires as input an array of arrays of points for these arguments,
    // allowing a caller to provide multiple frames from the same camera with corresponding points. For example, if
    // extremely high precision was required, many images could be taken with each camera, and findChessboardCorners
    // applied to each of those images, and OpenCV can jointly solve for all of the pairs of corresponding images.
    // However, to keep things simple, we use only one image from each device to calibrate.  This is also why each of
    // the vectors of corners is placed into another vector.
    //
    // A function in OpenCV's calibration function also requires that these points be F32 types, so we use those.
    // However, OpenCV still provides doubles as output, strangely enough.
    vector<vector<cv::Point3f>> chessboard_corners_world_nested_for_cv(1, chessboard_corners_world);
    vector<vector<cv::Point2f>> master_corners_nested_for_cv(1, master_chessboard_corners);
    vector<vector<cv::Point2f>> sub_corners_nested_for_cv(1, sub_chessboard_corners);

    cv::Matx33f master_camera_matrix = calibration_to_color_camera_matrix(master_calib);
    cv::Matx33f sub_camera_matrix = calibration_to_color_camera_matrix(sub_calib);
    vector<float> master_dist_coeff = calibration_to_color_camera_dist_coeffs(master_calib);
    vector<float> sub_dist_coeff = calibration_to_color_camera_dist_coeffs(sub_calib);

    // Finally, we'll actually calibrate the cameras.
    // Pass subordinate first, then master, because we want a transform from subordinate to master.
    Transformation tr;
    double error = cv::stereoCalibrate(chessboard_corners_world_nested_for_cv,
                                       sub_corners_nested_for_cv,
                                       master_corners_nested_for_cv,
                                       sub_camera_matrix,
                                       sub_dist_coeff,
                                       master_camera_matrix,
                                       master_dist_coeff,
                                       image_size,
                                       tr.R, // output
                                       tr.t, // output
                                       cv::noArray(),
                                       cv::noArray(),
                                       cv::CALIB_FIX_INTRINSIC | cv::CALIB_RATIONAL_MODEL | cv::CALIB_CB_FAST_CHECK);
    cout << "Finished calibrating!\n";
    cout << "Got error of " << error << "\n";
    return tr;
}

// The following functions provide the configurations that should be used for each camera.
// NOTE: Both cameras must have the same configuration (framerate, resolution, color and depth modes TODO what exactly
// needs to be the same
k4a_device_configuration_t get_master_config()
{
    k4a_device_configuration_t camera_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    camera_config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    camera_config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    camera_config.depth_mode = K4A_DEPTH_MODE_WFOV_UNBINNED; // no need for depth during calibration
    camera_config.camera_fps = K4A_FRAMES_PER_SECOND_15;
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
    camera_config.subordinate_delay_off_master_usec = 0; // Must be zero- this device is the master, so delay is 0.
    // Let half of the time needed for the depth cameras to not interfere with one another pass here (the other half is
    // in the master to subordinate delay)
    camera_config.depth_delay_off_color_usec = -static_cast<int32_t>(MIN_TIME_BETWEEN_DEPTH_CAMERA_PICTURES_USEC / 2);
    camera_config.synchronized_images_only = true;
    return camera_config;
}

k4a_device_configuration_t get_sub_config()
{
    k4a_device_configuration_t camera_config = get_master_config();
    // The color camera must be running for synchronization to work, even though we don't really use it
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
    camera_config.subordinate_delay_off_master_usec = 0; // sync up the color cameras to help calibration
    // Only account for half of the delay here. The other half comes from the master depth camera capturing before the
    // master color camera.
    camera_config.depth_delay_off_color_usec = MIN_TIME_BETWEEN_DEPTH_CAMERA_PICTURES_USEC / 2;
    camera_config.synchronized_images_only = true;
    return camera_config;
}

Transformation calibrate_devices(MultiDeviceCapturer &capturer,
                                 vector<k4a_device_configuration_t> &configs,
                                 const cv::Size &chessboard_pattern,
                                 float chessboard_square_length)
{
    k4a::calibration master_calibration = capturer.devices[0].get_calibration(configs[0].depth_mode,
                                                                              configs[0].color_resolution);
    k4a::calibration sub_calibration = capturer.devices[1].get_calibration(configs[1].depth_mode,
                                                                           configs[1].color_resolution);
    while (true)
    {
        vector<k4a::capture> captures = capturer.get_synchronized_captures(configs);
        k4a::capture &master_capture = captures[0];
        k4a::capture &sub_capture = captures[1];
        // get_color_image is guaranteed to be non-null because we use get_synchronized_captures for color
        // (get_synchronized_captures also offers a flag to use depth for the subordinate camera instead of color).
        k4a::image master_color_image = master_capture.get_color_image();
        k4a::image sub_color_image = sub_capture.get_color_image();
        cv::Mat cv_master_color_image = k4a_color_to_opencv(master_color_image);
        cv::Mat cv_sub_color_image = k4a_color_to_opencv(sub_color_image);
        vector<cv::Point2f> master_chessboard_corners;
        vector<cv::Point2f> sub_chessboard_corners;

        bool ready_to_calibrate = find_chessboard_corners_helper(cv_master_color_image,
                                                                 cv_sub_color_image,
                                                                 chessboard_pattern,
                                                                 master_chessboard_corners,
                                                                 sub_chessboard_corners);

        if (ready_to_calibrate)
        {
            return stereo_calibration(master_calibration,
                                      sub_calibration,
                                      master_chessboard_corners,
                                      sub_chessboard_corners,
                                      cv_master_color_image.size(),
                                      chessboard_pattern,
                                      chessboard_square_length);
        }
    }
}

k4a::image create_depth_image_like(const k4a::image &im)
{
    return k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16,
                              im.get_width_pixels(),
                              im.get_height_pixels(),
                              im.get_width_pixels() * static_cast<int>(sizeof(uint16_t)));
}

int main(int argc, char **argv)
{
    float chessboard_square_length = 0.; // Must be included in the input params
    int32_t color_exposure_usec = 8000;  // somewhat reasonable default exposure time
    int32_t powerline_freq = 2;          // default to a 60 Hz powerline
    cv::Size chessboard_pattern(0, 0);   // height, width. Both need to be set.

    static struct option long_options[] = { { "board-height", required_argument, 0, 'h' },
                                            { "board-width", required_argument, 0, 'w' },
                                            { "board-square-length", required_argument, 0, 's' },
                                            { "powerline-frequency", required_argument, 0, 'f' },
                                            { "color-exposure", required_argument, 0, 'e' },
                                            { "depth-threshold", required_argument, 0, 't' },
                                            { 0, 0, 0, 0 } };
    int option_index = 0;
    int input_opt = 0;
    uint16_t depth_threshold = 1000; // default to 1 meter
    while ((input_opt = getopt_long(argc, argv, "h:w:s:f:e:t:", long_options, &option_index)) != -1)
    {
        switch (input_opt)
        {
        case 'h':
            chessboard_pattern.height = std::stoi(optarg);
            break;
        case 'w':
            chessboard_pattern.width = std::stoi(optarg);
            break;
        case 's':
            chessboard_square_length = std::stof(optarg);
            break;
        case 'f':
            powerline_freq = std::stoi(optarg);
            break;
        case 'e':
            color_exposure_usec = std::stoi(optarg);
            break;
        case 't':
            depth_threshold = std::stoi(optarg);
            break;
        case '?':
            throw std::runtime_error("Incorrect arguments");
            break;
        default:
            throw std::runtime_error(std::string("getopt returned unexpected character code, ") +
                                     std::to_string(input_opt));
        }
    }

    if (chessboard_pattern.height == 0)
    {
        throw std::runtime_error("Chessboard height is not properly set! Use -h");
    }
    if (chessboard_pattern.width == 0)
    {
        throw std::runtime_error("Chessboard height is not properly set! Use -w");
    }
    if (chessboard_square_length == 0.)
    {
        throw std::runtime_error("Chessboard square size is not properly set! Use -s");
    }

    cout << "Chessboard height: " << chessboard_pattern.height << ". Chessboard width: " << chessboard_pattern.width
         << ". Chessboard square length: " << chessboard_square_length << endl;

    // Require 2 cameras
    const int num_devices = k4a::device::get_installed_count();
    if (num_devices != 2)
    {
        throw std::runtime_error("Exactly 2 cameras are required!");
    }

    // Set up a MultiDeviceCapturer to handle getting many synchronous captures
    vector<int> device_indices{ 0, 1 };
    MultiDeviceCapturer capturer(device_indices, color_exposure_usec, powerline_freq);

    // Create configurations. Note that the master MUST be first, and the order of indices in device_indices is not
    // necessarily preserved because the device_indices may not have ordered according to master and subordinate
    vector<k4a_device_configuration_t> configs{ get_master_config(), get_sub_config() };
    k4a_device_configuration_t &master_config = configs.front();
    k4a_device_configuration_t &sub_config = configs.back();

    capturer.start_devices(configs);

    // This wraps all the device-to-device details
    Transformation tr_color_sub_to_color_master =
        calibrate_devices(capturer, configs, chessboard_pattern, chessboard_square_length);

    k4a::calibration master_calibration = capturer.devices[0].get_calibration(master_config.depth_mode,
                                                                              master_config.color_resolution);
    k4a::calibration sub_calibration = capturer.devices[1].get_calibration(sub_config.depth_mode,
                                                                           sub_config.color_resolution);
    // Get the transformation from subordinate depth to subordinate color using its calibration object
    Transformation tr_depth_sub_to_color_sub = calibration_to_depth_to_color_transformation(sub_calibration);

    // We now have the subordinate depth to subordinate color transform. We also have the transformation from the
    // subordinate color perspective to the master color perspective from the calibration earlier. Now let's compose the
    // depth sub -> color sub, color sub -> color master into depth sub -> color master
    Transformation tr_depth_sub_to_color_master = tr_depth_sub_to_color_sub.compose_with(tr_color_sub_to_color_master);

    // Now, we're going to set up the transformations. DO THIS OUTSIDE OF YOUR MAIN LOOP! Constructing transformations
    // does a lot of preemptive work to make the transform as fast as possible.
    k4a::transformation master_depth_to_master_color(master_calibration);

    // We're going to update the existing calibration extrinsics on getting from the sub depth camera to the sub color
    // camera, overwriting it with the transformation to get from the sub depth camera to the master color camera
    set_calibration_depth_to_color_from_transformation(sub_calibration, tr_depth_sub_to_color_master);
    k4a::transformation sub_depth_to_master_color(sub_calibration);

    while (true)
    {
        vector<k4a::capture> captures = capturer.get_synchronized_captures(configs,
                                                                           true); // This is to get the depth image for
                                                                                  // the subordinate, not the color
        k4a::capture &master_capture = captures[0];
        k4a::capture &sub_capture = captures[1];

        k4a::image master_color_image = master_capture.get_color_image();
        k4a::image master_depth_image = master_capture.get_depth_image();
        k4a::image sub_depth_image = sub_capture.get_depth_image();

        // let's greenscreen out things that are far away.
        // first: let's get the master depth image into the color camera space
        // create a copy with the same parameters
        k4a::image k4a_master_depth_in_master_color = create_depth_image_like(master_color_image);

        master_depth_to_master_color.depth_image_to_color_camera(master_depth_image, &k4a_master_depth_in_master_color);

        // now, create an OpenCV version of the depth matrix for easy usage
        cv::Mat opencv_master_depth_in_master_color = k4a_depth_to_opencv(k4a_master_depth_in_master_color);

        // Finally, it's time to create the image for the depth image in the master color perspective
        k4a::image k4a_sub_depth_in_master_color = create_depth_image_like(master_color_image);
        sub_depth_to_master_color.depth_image_to_color_camera(sub_depth_image, &k4a_sub_depth_in_master_color);

        cv::Mat opencv_sub_depth_in_master_color = k4a_depth_to_opencv(k4a_sub_depth_in_master_color);
        cv::Mat normalized_opencv_sub_depth_in_master_color;
        cv::normalize(opencv_sub_depth_in_master_color,
                      normalized_opencv_sub_depth_in_master_color,
                      0,
                      256,
                      cv::NORM_MINMAX);
        cv::Mat grayscale_opencv_sub_depth_in_master_color;
        normalized_opencv_sub_depth_in_master_color.convertTo(grayscale_opencv_sub_depth_in_master_color, CV_32FC3);
        cv::imshow("Subordinate depth in master color", grayscale_opencv_sub_depth_in_master_color);
        cv::waitKey(1);

        cv::Mat master_opencv_color_image = k4a_color_to_opencv(master_color_image);

        // create the image that will be be used as output
        cv::Mat output_image(master_opencv_color_image.rows,
                             master_opencv_color_image.cols,
                             CV_32FC3,
                             cv::Scalar(0, 0, 0));

        cv::Mat master_image_float;
        master_opencv_color_image.convertTo(master_image_float, CV_32F);
        master_image_float = master_image_float / 255.0;
        cv::Mat master_valid_mask;
        cv::bitwise_and(opencv_master_depth_in_master_color != 0,
                        opencv_master_depth_in_master_color < depth_threshold,
                        master_valid_mask);
        cv::Mat sub_valid_mask;
        cv::bitwise_and(opencv_sub_depth_in_master_color != 0,
                        opencv_sub_depth_in_master_color < depth_threshold,
                        sub_valid_mask);
        cv::Mat output = master_image_float;
        // cv::add(output, cv::Scalar(0, .75, 0), output, ~sub_valid_mask);
        // cv::add(output, cv::Scalar(0, 0, .75), output, ~master_valid_mask);
        cv::add(output, cv::Scalar(0, 0, .75), output, ~sub_valid_mask & ~master_valid_mask);
        cv::imshow("Greenscreened", output);
        cv::waitKey(1);
    }
}
