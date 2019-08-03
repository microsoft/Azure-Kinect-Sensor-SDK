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

// This is the maximum difference between when we expected an image's timestamp to be and when it actually occurred.
constexpr std::chrono::microseconds MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP(50);

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

cv::Mat k4a_calibration_to_color_camera_matrix(const k4a::calibration &cal)
{
    const k4a_calibration_intrinsic_parameters_t::_param &i = cal.color_camera_calibration.intrinsics.parameters.param;
    cv::Mat camera_matrix = cv::Mat::zeros(3, 3, CV_32F);
    camera_matrix.at<float>(0, 0) = i.fx;
    camera_matrix.at<float>(1, 1) = i.fx;
    camera_matrix.at<float>(0, 2) = i.cx;
    camera_matrix.at<float>(1, 2) = i.cy;
    camera_matrix.at<float>(2, 2) = 1;
    return camera_matrix;
}

cv::Mat construct_homogeneous(const cv::Mat &R, const cv::Vec3d &t)
{
    if (R.type() != CV_64F)
    {
        cout << "Must be 64F" << endl;
        exit(1);
    }
    cv::Mat homog_matrix = cv::Mat::zeros(cv::Size(4, 4), CV_64F);
    for (int i = 0; i < R.rows; ++i)
    {
        for (int j = 0; j < R.cols; ++j)
        {
            homog_matrix.at<double>(i, j) = R.at<double>(i, j);
        }
    }
    for (int i = 0; i < t.channels; ++i)
    {
        homog_matrix.at<double>(i, 3) = t[i];
    }
    homog_matrix.at<double>(3, 3) = 1;
    return homog_matrix;
}

std::tuple<cv::Mat, cv::Vec3d> deconstruct_homogeneous(const cv::Mat &H)
{
    cv::Mat R = cv::Mat::zeros(cv::Size(3, 3), CV_64F);
    cv::Vec3d t;
    if (H.size() != cv::Size(4, 4) || H.at<double>(3, 0) != 0 || H.at<double>(3, 1) != 0 || H.at<double>(3, 2) != 0 ||
        H.at<double>(3, 3) != 1)
    {
        throw std::runtime_error("Please use a valid homogeneous matrix.");
    }

    for (int i = 0; i < R.rows; ++i)
    {
        for (int j = 0; j < R.cols; ++j)
        {
            R.at<double>(i, j) = H.at<double>(i, j);
        }
    }
    for (size_t i = 0; i < t.channels; ++i)
    {
        t[i] = H.at<double>(i, 3);
    }
    return std::make_tuple(R, t);
}

std::tuple<cv::Mat, cv::Vec3d> k4a_calibration_to_depth_to_color_R_t(const k4a::calibration &cal)
{
    const k4a_calibration_extrinsics_t &ex = cal.extrinsics[K4A_CALIBRATION_TYPE_DEPTH][K4A_CALIBRATION_TYPE_COLOR];
    cv::Mat R = cv::Mat(3, 3, CV_64F);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            R.at<double>(i, j) = ex.rotation[i * 3 + j];
        }
    }
    cv::Vec3d t = cv::Vec3d(ex.translation[0], ex.translation[1], ex.translation[2]);
    return std::make_tuple(R, t);
}

void set_k4a_calibration_depth_to_color_from_R_t(k4a::calibration &cal, const cv::Mat &R, const cv::Vec3d &t)
{
    k4a_calibration_extrinsics_t &ex = cal.extrinsics[K4A_CALIBRATION_TYPE_DEPTH][K4A_CALIBRATION_TYPE_COLOR];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            ex.rotation[i * 3 + j] = static_cast<float>(R.at<double>(i, j));
        }
    }
    for (int i = 0; i < 3; ++i)
    {
        ex.translation[i] = static_cast<float>(t[i]);
    }
}

vector<float> k4a_calibration_to_color_camera_dist_coeffs(const k4a::calibration &cal)
{
    const k4a_calibration_intrinsic_parameters_t::_param &i = cal.color_camera_calibration.intrinsics.parameters.param;
    return { i.k1, i.k2, i.p1, i.p2, i.k3, i.k4, i.k5, i.k6 };
}

// Blocks until we have synchronized captures stored in master_capture and sub_capture
std::tuple<k4a::capture, k4a::capture> get_synchronized_captures(k4a::device &master,
                                                                 k4a::device &subordinate,
                                                                 k4a_device_configuration_t &sub_config,
                                                                 bool compare_sub_depth_instead_of_color = false)
{
    // Dealing with the synchronized cameras is complex. The Azure Kinect DK:
    //      (a) does not guarantee exactly equal timestamps between depth and color or between cameras (delays can be
    //          configured but timestamps will only be approximately the same)
    //      (b) does not guarantee that, if the two most recent images were synchronized, that calling get_capture just
    //          once on each camera will still be synchronized.
    // There are several reasons for all of this. Internally, devices keep a queue of a few of the captured images and
    // serve those images as requested by get_capture(). However, images can also be dropped at any moment, and one
    // device may have more images ready than another device at a given moment, et cetera.
    //
    // Also, the process of synchronizing is complex. The cameras are not guaranteed to exactly match in all of their
    // timestamps when synchronized (though they should be very close). All delays are relative to the master camera's
    // color camera. To deal with these complexities, we employ a fairly straightforward algorithm. Start by reading in
    // two captures, then if the camera images were not taken at roughly the same time read a new one from the device
    // that had the older capture until the timestamps roughly match.

    // The captures used in the loop are outside of it so that they can persist across loop iterations. This is
    // necessary because each time this loop runs we'll only update the older capture.
    k4a::capture master_capture, sub_capture;
    bool master_success = master.get_capture(&master_capture, std::chrono::milliseconds{ 5000 }); // 5 sec timeout
    bool sub_success = subordinate.get_capture(&sub_capture, std::chrono::milliseconds{ 5000 });  // 5 sec timeout
    if (!master_success || !sub_success)
    {
        throw std::runtime_error("Getting a capture timed out!");
    }

    bool have_synced_images = false;
    while (!have_synced_images)
    {
        k4a::image master_color_image = master_capture.get_color_image();
        k4a::image sub_image;
        if (compare_sub_depth_instead_of_color)
        {
            sub_image = sub_capture.get_depth_image();
        }
        else
        {
            sub_image = sub_capture.get_color_image();
        }

        if (master_color_image && sub_image)
        {
            std::chrono::microseconds sub_image_time = sub_image.get_device_timestamp();
            std::chrono::microseconds master_color_image_time = master_color_image.get_device_timestamp();
            // The subordinate's color image timestamp, ideally, is the master's color image timestamp plus the delay we
            // configured between the master device color camera and subordinate device color camera
            std::chrono::microseconds expected_sub_image_time = master_color_image_time + std::chrono::microseconds{
                sub_config.subordinate_delay_off_master_usec
            };
            std::chrono::microseconds sub_image_time_error = sub_image_time - expected_sub_image_time;
            // The time error's absolute value must be within the permissible range. So, for example, if
            // MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP is 2, offsets of -2, -1, 0, 1, and -2 are permitted
            if (sub_image_time_error < -MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP)
            {
                // Example, where MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP is 1
                // time                    t=1  t=2  t=3
                // actual timestamp        x    .    .
                // expected timestamp      .    .    x
                // error: 1 - 3 = -2, which is less than the worst-case-allowable offset of -1
                // the subordinate camera image timestamp was earlier than it is allowed to be. This means the
                // subordinate is lagging and we need to update the subordinate to get the subordinate caught up
                cout << "Subordinate lagging...\n";
                subordinate.get_capture(&sub_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
            }
            else if (sub_image_time_error > MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP)
            {
                // Example, where MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP is 1
                // time                    t=1  t=2  t=3
                // actual timestamp        .    .    x
                // expected timestamp      x    .    .
                // error: 3 - 1 = 2, which is more than the worst-case-allowable offset of 1
                // the subordinate camera image timestamp was later than it is allowed to be. This means the subordinate
                // is ahead and we need to update the master to get the master caught up
                cout << "Master lagging...\n";
                master.get_capture(&master_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
            }
            else
            {
                // The captures are sufficiently synchronized. Exit the function.
                have_synced_images = true;
            }
        }
        else
        {
            // One of the captures or one of the images are bad, so just replace both. One could make this more
            // sophisticated and try to only to only replace one of these captures, et cetera, to try to keep a good one
            // but we'll keep things simple and just throw both away and try again.
            // If this is happening, it's likely the cameras are improperly configured and frames aren't synchronized
            cout << "One of the images was bad!\n";
            master.get_capture(&master_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
            subordinate.get_capture(&sub_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
        }
    }
    return std::make_tuple(master_capture, sub_capture);
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

std::tuple<cv::Mat, cv::Vec3d> stereo_calibration(const k4a::calibration &master_calib,
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

    cv::Mat master_camera_matrix = k4a_calibration_to_color_camera_matrix(master_calib);
    cv::Mat sub_camera_matrix = k4a_calibration_to_color_camera_matrix(sub_calib);
    vector<float> master_dist_coeff = k4a_calibration_to_color_camera_dist_coeffs(master_calib);
    vector<float> sub_dist_coeff = k4a_calibration_to_color_camera_dist_coeffs(sub_calib);

    // Finally, we'll actually calibrate the cameras.
    // Pass subordinate first, then master, because we want a transform from subordinate to master.
    cv::Mat R;
    cv::Vec3d t;
    double error = cv::stereoCalibrate(chessboard_corners_world_nested_for_cv,
                                       sub_corners_nested_for_cv,
                                       master_corners_nested_for_cv,
                                       sub_camera_matrix,
                                       sub_dist_coeff,
                                       master_camera_matrix,
                                       master_dist_coeff,
                                       image_size,
                                       R, // output
                                       t, // output
                                       cv::noArray(),
                                       cv::noArray(),
                                       cv::CALIB_FIX_INTRINSIC | cv::CALIB_RATIONAL_MODEL | cv::CALIB_CB_FAST_CHECK);
    cout << "Finished calibrating!\n";
    cout << "Got error of " << error << "\n";
    return std::make_tuple(R, t);
}

// The following few functions provide the configurations that should be used for each camera in each case.
// NOTE: Both cameras must have the same configuration (framerate, resolution, color and depth modes TODO what exactly
// needs to be the same
k4a_device_configuration_t get_master_config_calibration()
{
    k4a_device_configuration_t camera_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    camera_config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    camera_config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    camera_config.depth_mode = K4A_DEPTH_MODE_OFF; // no need for depth during calibration
    camera_config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
    camera_config.subordinate_delay_off_master_usec = 0; // Must be zero- this device is the master, so delay is 0.
    camera_config.depth_delay_off_color_usec = 0;
    camera_config.synchronized_images_only = false;
    return camera_config;
}

k4a_device_configuration_t get_sub_config_calibration()
{
    k4a_device_configuration_t camera_config = get_master_config_calibration();
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
    // Note that subordinate_delay_off_master_usec is still 0, because we want the master camera and subordinate camera
    // to calibrate with pictures that were taken at the same time.
    return camera_config;
}

k4a_device_configuration_t get_master_config_green_screen()
{
    k4a_device_configuration_t camera_config = get_master_config_calibration();
    camera_config.depth_mode = K4A_DEPTH_MODE_WFOV_UNBINNED; // need to have depth to green screen
    camera_config.camera_fps = K4A_FRAMES_PER_SECOND_15;     // 15 FPS is the max for the wide field of view mode. The
                                                             // device can go up to 30 FPS with the narrow field of view
                                                             // mode
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
    camera_config.subordinate_delay_off_master_usec = 0; // Must be zero- this device is the master, so delay is 0.
    // Let half of the time needed for the depth cameras to not interfere with one another pass here (the other half is
    // in the master to subordinate delay)
    camera_config.depth_delay_off_color_usec = -static_cast<int32_t>(MIN_TIME_BETWEEN_DEPTH_CAMERA_PICTURES_USEC / 2);
    camera_config.synchronized_images_only = true;
    return camera_config;
}

k4a_device_configuration_t get_sub_config_green_screen()
{
    k4a_device_configuration_t camera_config = get_master_config_green_screen();
    // The color camera must be running for synchronization to work
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
    // Only account for half of the delay here. The other half comes from the master depth camera capturing before the
    // master color camera.
    camera_config.subordinate_delay_off_master_usec = MIN_TIME_BETWEEN_DEPTH_CAMERA_PICTURES_USEC / 2;
    camera_config.depth_delay_off_color_usec = 0; // TODO TEST this- ignored, or, more likely, ruins everything
    camera_config.synchronized_images_only = true;
    return camera_config;
}

std::tuple<cv::Mat, cv::Vec3d> calibrate_devices(k4a::device &master,
                                                 k4a::device &subordinate,
                                                 k4a_device_configuration_t &master_calibration_config,
                                                 k4a_device_configuration_t &sub_calibration_config,
                                                 const cv::Size &chessboard_pattern,
                                                 float chessboard_square_length)
{
    k4a::calibration master_calibration = master.get_calibration(master_calibration_config.depth_mode,
                                                                 master_calibration_config.color_resolution);
    k4a::calibration sub_calibration = subordinate.get_calibration(sub_calibration_config.depth_mode,
                                                                   sub_calibration_config.color_resolution);
    cv::Mat R;
    cv::Vec3d t;
    bool calibrated = false;
    while (!calibrated)
    {
        k4a::capture master_capture, sub_capture;
        std::tie(master_capture, sub_capture) = get_synchronized_captures(master, subordinate, sub_calibration_config);
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
            std::tie(R, t) = stereo_calibration(master_calibration,
                                                sub_calibration,
                                                master_chessboard_corners,
                                                sub_chessboard_corners,
                                                cv_master_color_image.size(),
                                                chessboard_pattern,
                                                chessboard_square_length);
            calibrated = true;
        }
    }
    return std::make_tuple(R, t);
}

std::tuple<cv::Mat, cv::Vec3d>
compose_calibration_transforms(const cv::Mat &R_1, const cv::Vec3d &t_1, const cv::Mat &R_2, const cv::Vec3d &t_2)
{
    cv::Mat H_1 = construct_homogeneous(R_1, t_1);
    cv::Mat H_2 = construct_homogeneous(R_2, t_2);
    cv::Mat H_3 = H_1 * H_2;
    cv::Mat R_3;
    cv::Vec3d t_3;
    std::tie(R_3, t_3) = deconstruct_homogeneous(H_3);
    return std::make_tuple(R_3, t_3);
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
    float chessboard_square_length = 0.; // Must be set
    int32_t color_exposure_usec = 8000;  // somewhat reasonable default exposure time
    int32_t powerline_freq = 2;          // default to a 60 Hz powerline
    cv::Size chessboard_pattern(0, 0);   // height, width. Both need to be set.

    static struct option long_options[] = { { "board-height", required_argument, 0, 'h' },
                                            { "board-width", required_argument, 0, 'w' },
                                            { "board-square-length", required_argument, 0, 's' },
                                            { "powerline-frequency", required_argument, 0, 'f' },
                                            { "color-exposure", required_argument, 0, 'e' },
                                            { 0, 0, 0, 0 } };
    int option_index = 0;
    int input_opt = 0;
    while ((input_opt = getopt_long(argc, argv, "h:w:s:f:e:", long_options, &option_index)) != -1)
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
    // Find out which device is the master and which is the subordinate. We'll assume that the master camera is the
    // first one with a cable in its sync out port.
    k4a::device master = k4a::device::open(0); // Assume device 0 is the master; if it's not the master, we'll swap it
    k4a::device subordinate = k4a::device::open(1);

    if (master.is_sync_out_connected())
    {
        // The first device has sync out connected (it can send commands), so it's the master. No need to swap.
    }
    else if (subordinate.is_sync_out_connected())
    {
        // The second device has its sync out connected, so it's actually the master. We need to swap.
        std::swap(master, subordinate);
    }
    else
    {
        // Neither device has sync out connected!
        throw std::runtime_error("Neither camera has the sync out port connected, so neither device can be master!");
    }
    // make sure that subordinate has sync in connected
    if (!subordinate.is_sync_in_connected())
    {
        throw std::runtime_error("Subordinate camera does not have the sync in port connected!");
    }

    // If you want to synchronize cameras, you need to manually set both their exposures
    master.set_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                             K4A_COLOR_CONTROL_MODE_MANUAL,
                             color_exposure_usec);
    subordinate.set_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                  K4A_COLOR_CONTROL_MODE_MANUAL,
                                  color_exposure_usec);

    // This setting compensates for the flicker of lights due to the frequency of AC power in your region. If you are in
    // an area with 50 Hz power, this may need to be updated (check the docs for k4a_color_control_command_t)
    master.set_color_control(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, K4A_COLOR_CONTROL_MODE_MANUAL, powerline_freq);
    subordinate.set_color_control(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, K4A_COLOR_CONTROL_MODE_MANUAL, powerline_freq);

    // Construct the calibrations that these types will use
    k4a_device_configuration_t master_calibration_config = get_master_config_calibration();
    k4a_device_configuration_t sub_calibration_config = get_sub_config_calibration();

    // now it's time to start the cameras. Start the subordinate first, because it needs to be ready to receive commands
    // when the master starts up.
    subordinate.start_cameras(&sub_calibration_config);
    master.start_cameras(&master_calibration_config);

    // These will hold the rotation and translation we get from calibration
    cv::Mat R_color_sub_to_color_master;
    cv::Vec3d t_color_sub_to_color_master;

    // This wraps all the calibration details
    std::tie(R_color_sub_to_color_master, t_color_sub_to_color_master) = calibrate_devices(master,
                                                                                           subordinate,
                                                                                           master_calibration_config,
                                                                                           sub_calibration_config,
                                                                                           chessboard_pattern,
                                                                                           chessboard_square_length);

    // End calibration
    master.stop_cameras();
    subordinate.stop_cameras();

    k4a_device_configuration_t master_config = get_master_config_green_screen();
    k4a_device_configuration_t sub_config = get_sub_config_green_screen();
    k4a::calibration master_calibration = master.get_calibration(master_config.depth_mode,
                                                                 master_config.color_resolution);
    k4a::calibration sub_calibration = subordinate.get_calibration(sub_config.depth_mode, sub_config.color_resolution);
    // Get the transformation from subordinate depth to subordinate color using its calibration object
    cv::Mat R_depth_sub_to_color_sub;
    cv::Vec3d t_depth_sub_to_color_sub;
    std::tie(R_depth_sub_to_color_sub,
             t_depth_sub_to_color_sub) = k4a_calibration_to_depth_to_color_R_t(sub_calibration);

    // We now have the subordinate depth to subordinate color transform. We also have the transformation from the
    // subordinate color perspective to the master color perspective from the calibration earlier. Now let's compose the
    // depth sub -> color sub, color sub -> color master into depth sub -> color master
    cv::Mat R_depth_sub_to_color_master;
    cv::Vec3d t_depth_sub_to_color_master;
    std::tie(R_depth_sub_to_color_master,
             t_depth_sub_to_color_master) = compose_calibration_transforms(R_depth_sub_to_color_sub,
                                                                           t_depth_sub_to_color_sub,
                                                                           R_color_sub_to_color_master,
                                                                           t_color_sub_to_color_master);

    // Now, we're going to set up the transformations. DO THIS OUTSIDE OF YOUR MAIN LOOP! Constructing transformations
    // does a lot of preemptive work to make the transform as fast as possible.
    k4a::transformation master_depth_to_master_color(master_calibration);
    k4a::transformation sub_depth_to_sub_color(sub_calibration);

    // Now it's time to get clever. We're going to update the existing calibration extrinsics on getting from the sub
    // depth camera to the sub color camera, overwriting it with the transformation to get from the sub depth camera to
    // the master color camera
    set_k4a_calibration_depth_to_color_from_R_t(sub_calibration,
                                                R_depth_sub_to_color_master,
                                                t_depth_sub_to_color_master);
    k4a::transformation sub_depth_to_master_color(sub_calibration);

    // Re-start the cameras with the new configurations to get new configurations with the green screen configs
    // don't forget that subordinate still needs to go first
    subordinate.start_cameras(&sub_config);
    master.start_cameras(&master_config);

    while (true)
    {
        k4a::capture master_capture;
        k4a::capture sub_capture;
        std::tie(master_capture, sub_capture) = get_synchronized_captures(master,
                                                                          subordinate,
                                                                          sub_config,
                                                                          true); // This is to get the depth image for
                                                                                 // the subordinate, not the color

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

        // now let's get the subordinate depth image into the subordinate color space
        k4a::image k4a_sub_depth_in_sub_color = create_depth_image_like(master_color_image);
        sub_depth_to_sub_color.depth_image_to_color_camera(sub_depth_image, &k4a_sub_depth_in_sub_color);
        cv::Mat opencv_sub_depth_in_sub_color = k4a_depth_to_opencv(k4a_sub_depth_in_sub_color);

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

        const uint16_t THRESHOLD = 1200; // TODO what should this be
        cv::Mat master_image_float;
        master_opencv_color_image.convertTo(master_image_float, CV_32F);
        master_image_float = master_image_float / 255.0;
        cv::Mat master_valid_mask;
        cv::bitwise_and(opencv_master_depth_in_master_color != 0,
                        opencv_master_depth_in_master_color < THRESHOLD,
                        master_valid_mask);
        cv::Mat sub_valid_mask;
        cv::bitwise_and(opencv_sub_depth_in_master_color != 0,
                        opencv_sub_depth_in_master_color < THRESHOLD,
                        sub_valid_mask);
        cv::Mat output = master_image_float;
        // cv::add(output, cv::Scalar(0, .75, 0), output, ~sub_valid_mask);
        // cv::add(output, cv::Scalar(0, 0, .75), output, ~master_valid_mask);
        cv::add(output, cv::Scalar(0, 0, .75), output, ~sub_valid_mask & ~master_valid_mask);
        cv::imshow("Greenscreened", output);
        cv::waitKey(1);
    }
}
