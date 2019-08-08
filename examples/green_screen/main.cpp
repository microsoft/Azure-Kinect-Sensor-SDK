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
cv::Mat color_to_opencv(const k4a::image &im)
{
    // TODO this only handles mjpg
    cv::Mat raw_data(1, im.get_size(), CV_8UC1, (void *)im.get_buffer());
    cv::Mat decoded = cv::imdecode(raw_data, cv::IMREAD_COLOR);
    return decoded;
}

cv::Mat depth_to_opencv(const k4a::image &im)
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

Transformation get_depth_to_color_transformation_from_calibration(const k4a::calibration &cal)
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
bool find_chessboard_corners_helper(cv::Mat main_color_image,
                                    cv::Mat backup_color_image,
                                    const cv::Size &chessboard_pattern,
                                    vector<cv::Point2f> &main_chessboard_corners,
                                    vector<cv::Point2f> &backup_chessboard_corners)
{
    bool found_chessboard_main = cv::findChessboardCorners(main_color_image,
                                                           chessboard_pattern,
                                                           main_chessboard_corners,
                                                           cv::CALIB_CB_FAST_CHECK);
    bool found_chessboard_backup = cv::findChessboardCorners(backup_color_image,
                                                             chessboard_pattern,
                                                             backup_chessboard_corners,
                                                             cv::CALIB_CB_FAST_CHECK);

    // Cover the failure cases where chessboards were not found in one or both images.
    if (!found_chessboard_main || !found_chessboard_backup)
    {
        if (found_chessboard_main)
        {
            cout << "Could not find the chessboard corners in the backup image. Trying again...\n";
        }
        // Likewise, if the chessboard was found in the backup image, it was not found in the main image.
        else if (found_chessboard_backup)
        {
            cout << "Could not find the chessboard corners in the main image. Trying again...\n";
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

    cv::Vec2f main_image_corners_vec = main_chessboard_corners.back() - main_chessboard_corners.front();
    cv::Vec2f backup_image_corners_vec = backup_chessboard_corners.back() - backup_chessboard_corners.front();
    if (main_image_corners_vec.dot(backup_image_corners_vec) <= 0.0)
    {
        std::reverse(backup_chessboard_corners.begin(), backup_chessboard_corners.end());
    }

    // Comment out this section to not show the calibration output
    cv::drawChessboardCorners(main_color_image, chessboard_pattern, main_chessboard_corners, true);
    cv::drawChessboardCorners(backup_color_image, chessboard_pattern, backup_chessboard_corners, true);
    cv::imshow("Chessboard view from main camera", main_color_image);
    cv::waitKey(1);
    cv::imshow("Chessboard view from backup camera", backup_color_image);
    cv::waitKey(1);

    return true;
}

Transformation stereo_calibration(const k4a::calibration &main_calib,
                                  const k4a::calibration &backup_calib,
                                  const vector<vector<cv::Point2f>> &main_chessboard_corners_list,
                                  const vector<vector<cv::Point2f>> &backup_chessboard_corners_list,
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
    vector<vector<cv::Point3f>> chessboard_corners_world_nested_for_cv(main_chessboard_corners_list.size(),
                                                                       chessboard_corners_world);

    cv::Matx33f main_camera_matrix = calibration_to_color_camera_matrix(main_calib);
    cv::Matx33f backup_camera_matrix = calibration_to_color_camera_matrix(backup_calib);
    vector<float> main_dist_coeff = calibration_to_color_camera_dist_coeffs(main_calib);
    vector<float> backup_dist_coeff = calibration_to_color_camera_dist_coeffs(backup_calib);

    // Finally, we'll actually calibrate the cameras.
    // Pass backup first, then main, because we want a transform from backup to main.
    Transformation tr;
    double error = cv::stereoCalibrate(chessboard_corners_world_nested_for_cv,
                                       backup_chessboard_corners_list,
                                       main_chessboard_corners_list,
                                       backup_camera_matrix,
                                       backup_dist_coeff,
                                       main_camera_matrix,
                                       main_dist_coeff,
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
k4a_device_configuration_t get_main_config()
{
    k4a_device_configuration_t camera_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    camera_config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    camera_config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    camera_config.depth_mode = K4A_DEPTH_MODE_WFOV_UNBINNED; // no need for depth during calibration
    camera_config.camera_fps = K4A_FRAMES_PER_SECOND_15;
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER; // main will be the master
    camera_config.subordinate_delay_off_master_usec = 0; // Must be zero- this device is the master, so delay is 0.
    // Let half of the time needed for the depth cameras to not interfere with one another pass here (the other half is
    // in the master to subordinate delay)
    camera_config.depth_delay_off_color_usec = -static_cast<int32_t>(MIN_TIME_BETWEEN_DEPTH_CAMERA_PICTURES_USEC / 2);
    camera_config.synchronized_images_only = true;
    return camera_config;
}

k4a_device_configuration_t get_backup_config()
{
    k4a_device_configuration_t camera_config = get_main_config();
    // The color camera must be running for synchronization to work, even though we don't really use it
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE; // backup will be the subordinate
    camera_config.subordinate_delay_off_master_usec = 0;             // sync up the color cameras to help calibration
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
    k4a::calibration main_calibration = capturer.devices[0].get_calibration(configs[0].depth_mode,
                                                                            configs[0].color_resolution);
    k4a::calibration backup_calibration = capturer.devices[1].get_calibration(configs[1].depth_mode,
                                                                              configs[1].color_resolution);
    vector<vector<cv::Point2f>> main_chessboard_corners_list;
    vector<vector<cv::Point2f>> backup_chessboard_corners_list;
    while (true)
    {
        vector<k4a::capture> captures = capturer.get_synchronized_captures(configs);
        k4a::capture &main_capture = captures[0];
        k4a::capture &backup_capture = captures[1];
        // get_color_image is guaranteed to be non-null because we use get_synchronized_captures for color
        // (get_synchronized_captures also offers a flag to use depth for the backup camera instead of color).
        k4a::image main_color_image = main_capture.get_color_image();
        k4a::image backup_color_image = backup_capture.get_color_image();
        cv::Mat cv_main_color_image = color_to_opencv(main_color_image);
        cv::Mat cv_backup_color_image = color_to_opencv(backup_color_image);

        vector<cv::Point2f> main_chessboard_corners;
        vector<cv::Point2f> backup_chessboard_corners;
        bool got_corners = find_chessboard_corners_helper(cv_main_color_image,
                                                          cv_backup_color_image,
                                                          chessboard_pattern,
                                                          main_chessboard_corners,
                                                          backup_chessboard_corners);
        if (got_corners)
        {
            main_chessboard_corners_list.emplace_back(main_chessboard_corners);
            backup_chessboard_corners_list.emplace_back(backup_chessboard_corners);
        }

        // Get 20 frames before doing calibration.
        if (main_chessboard_corners_list.size() >= 20)
        {
            cout << "Calculating calibration..." << endl;
            return stereo_calibration(main_calibration,
                                      backup_calibration,
                                      main_chessboard_corners_list,
                                      backup_chessboard_corners_list,
                                      cv_main_color_image.size(),
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
    float chessboard_square_length = 0.; // must be included in the input params
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

    // Create configurations. Note that the main MUST be first, and the order of indices in device_indices is not
    // necessarily preserved because the device_indices may not have ordered according to main and backup
    vector<k4a_device_configuration_t> configs{ get_main_config(), get_backup_config() };
    k4a_device_configuration_t &main_config = configs.front();
    k4a_device_configuration_t &backup_config = configs.back();

    capturer.start_devices(configs);

    // This wraps all the device-to-device details
    Transformation tr_color_backup_to_color_main =
        calibrate_devices(capturer, configs, chessboard_pattern, chessboard_square_length);

    k4a::calibration main_calibration = capturer.devices[0].get_calibration(main_config.depth_mode,
                                                                            main_config.color_resolution);
    k4a::calibration backup_calibration = capturer.devices[1].get_calibration(backup_config.depth_mode,
                                                                              backup_config.color_resolution);
    // Get the transformation from backup depth to backup color using its calibration object
    Transformation tr_depth_backup_to_color_backup = get_depth_to_color_transformation_from_calibration(
        backup_calibration);

    // We now have the backup depth to backup color transform. We also have the transformation from the
    // backup color perspective to the main color perspective from the calibration earlier. Now let's compose the
    // depth backup -> color backup, color backup -> color main into depth backup -> color main
    Transformation tr_depth_backup_to_color_main = tr_depth_backup_to_color_backup.compose_with(
        tr_color_backup_to_color_main);

    // Now, we're going to set up the transformations. DO THIS OUTSIDE OF YOUR MAIN LOOP! Constructing transformations
    // does a lot of preemptive work to make the transform as fast as possible.
    k4a::transformation main_depth_to_main_color(main_calibration);

    // We're going to update the existing calibration extrinsics on getting from the backup depth camera to the backup
    // color camera, overwriting it with the transformation to get from the backup depth camera to the main color camera
    // TODO this needs to be done by constructing a new calibration object, not modifying
    set_calibration_depth_to_color_from_transformation(backup_calibration, tr_depth_backup_to_color_main);
    backup_calibration.color_camera_calibration = main_calibration.color_camera_calibration;
    k4a::transformation backup_depth_to_main_color(backup_calibration);

    while (true)
    {
        vector<k4a::capture> captures = capturer.get_synchronized_captures(configs,
                                                                           true); // This is to get the depth image for
                                                                                  // the backup, not the color
        k4a::image main_color_image = captures[0].get_color_image();
        k4a::image main_depth_image = captures[0].get_depth_image();
        k4a::image backup_depth_image = captures[1].get_depth_image();

        // let's green screen out things that are far away.
        // first: let's get the main depth image into the color camera space
        k4a::image main_depth_in_main_color = create_depth_image_like(main_color_image);
        main_depth_to_main_color.depth_image_to_color_camera(main_depth_image, &main_depth_in_main_color);
        cv::Mat cv_main_depth_in_main_color = depth_to_opencv(main_depth_in_main_color);

        // Get the depth image in the main color perspective
        k4a::image backup_depth_in_main_color = create_depth_image_like(main_color_image);
        backup_depth_to_main_color.depth_image_to_color_camera(backup_depth_image, &backup_depth_in_main_color);
        cv::Mat cv_backup_depth_in_main_color = depth_to_opencv(backup_depth_in_main_color);

        cv::Mat cv_main_color_image = color_to_opencv(main_color_image);

        // create the image that will be be used as output
        cv::Mat output_image(cv_main_color_image.rows, cv_main_color_image.cols, CV_8UC3, cv::Scalar(0, 0, 0));

        // Now it's time to actually construct the green screen. Where the depth is 0, the camera doesn't know how far
        // away the object is because it didn't get a response at that point. That's where we'll try to fill in the gaps
        // with the other camera.
        cv::Mat main_valid_mask = cv_main_depth_in_main_color != 0;
        cv::Mat backup_valid_mask = cv_backup_depth_in_main_color != 0;
        cv::Mat within_threshold_range = (main_valid_mask & (cv_main_depth_in_main_color < depth_threshold)) |
                                         (~main_valid_mask & backup_valid_mask &
                                          (cv_backup_depth_in_main_color < depth_threshold));
        // make a green background
        cv::Mat output(cv_main_color_image.size(), cv_main_color_image.type(), cv::Scalar(0, 255, 0));
        // copy all valid output to it
        cv_main_color_image.copyTo(output, within_threshold_range);
        cv::imshow("Green Screen", output);
        cv::waitKey(1);
    }
}
