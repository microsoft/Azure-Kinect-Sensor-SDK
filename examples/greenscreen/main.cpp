#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

#include <k4a/k4a.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using std::cout;
using std::endl;
using std::vector;

// TODO explain
constexpr std::chrono::microseconds MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP(50);

constexpr size_t board_height = 8;       // TODO command line arg
constexpr size_t board_width = 6;        // TODO command line arg
constexpr float board_square_size = 33.; // TODO command line arg

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

// TODO this doesn't actually work that well and I'm not sure whether it will be useful.
cv::Mat k4a_ir_to_opencv(const k4a::image &im)
{
    // TODO the docs really need an explanation of K4A_IMAGE_FORMAT_IR16 that clearly explains what it is
    cout << im.get_height_pixels() << std::endl;
    cout << im.get_width_pixels() << std::endl;
    cv::Mat normalized(im.get_height_pixels(), im.get_width_pixels(), CV_16UC1);
    cv::Mat result(im.get_height_pixels(), im.get_width_pixels(), CV_8UC1);
    cv::normalize(k4a_depth_to_opencv(im), normalized, 1, 0, cv::NORM_MINMAX);
    normalized.convertTo(result, CV_8UC1);
    return result;
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

void deconstruct_homogeneous(const cv::Mat &H, cv::Mat &R, cv::Vec3d &t)
{
    if (H.size() != cv::Size(4, 4) || H.at<double>(3, 0) != 0 || H.at<double>(3, 1) != 0 || H.at<double>(3, 2) != 0 ||
        H.at<double>(3, 3) != 1)
    {
        cout << "Please use a valid homogeneous matrix." << endl;
        exit(1);
    }
    R = cv::Mat::zeros(cv::Size(3, 3), CV_64F);
    t[0] = t[1] = t[2] = 0;

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
}

void k4a_calibration_to_depth_to_color_R_t(const k4a::calibration &cal, cv::Mat &R, cv::Vec3d &t)
{
    const k4a_calibration_extrinsics_t &ex = cal.extrinsics[K4A_CALIBRATION_TYPE_DEPTH][K4A_CALIBRATION_TYPE_COLOR];
    R = cv::Mat(3, 3, CV_64F);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            // TODO is it row-major or column-major?
            R.at<double>(i, j) = ex.rotation[i * 3 + j];
        }
    }
    t = cv::Vec3d(ex.translation[0], ex.translation[1], ex.translation[2]);
}

void set_k4a_calibration_depth_to_color_from_R_t(k4a::calibration &cal, const cv::Mat &R, const cv::Vec3d &t)
{
    k4a_calibration_extrinsics_t &ex = cal.extrinsics[K4A_CALIBRATION_TYPE_DEPTH][K4A_CALIBRATION_TYPE_COLOR];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            // TODO is it row-major or column-major?
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
void get_synchronized_captures(k4a::device &master,
                               k4a::device &sub,
                               k4a_device_configuration_t &sub_config,
                               k4a::capture &master_capture,
                               k4a::capture &sub_capture,
                               bool compare_sub_depth_instead_of_color = false)
{
    // Dealing with the synchronized cameras is complex. The Azure Kinect DK:
    //      (a) does not guarantee calling get_capture() synchronously the images received will have TODO
    //      identical timestamps as ones received from another device
    //      (b) does not guarantee that the depth image and camera image from a single
    //      camera's capture have the same timestamp (this delay can be changed but it will still only be
    //      approximately the same)
    // There are several reasons for all of this. Internally, devices keep a queue of a few of the captured images
    // and serve those images as requested by get_capture(). However, images can also be dropped at any moment, and
    // one device may have more images ready than another device at a given moment, et cetera.
    //
    // Also, the process of synchronizing is complex. The cameras are not guaranteed to exactly match in all of
    // their timestamps when synchronized (though they should be very close). All delays are relative to the master
    // camera's color camera. To deal with these complexities, we employ a fairly straightforward algorithm. Start
    // by reading in two captures, then if the camera images were not taken at roughly the same time read a new one
    // from the device that had the older capture until the timestamps roughly match.

    // The captures used in the loop are outside of it so that they can persist across loop iterations. This is
    // necessary because each time this loop runs we'll only update the older capture.
    master.get_capture(&master_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
    sub.get_capture(&sub_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });

    bool have_synced_images = false;
    static std::chrono::microseconds last_master_depth = std::chrono::microseconds(0);
    static std::chrono::microseconds last_sub_depth = std::chrono::microseconds(0);
    while (!have_synced_images)
    {
        // TODO should these be references?
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
        // Debugging code left in in case this part is giving you trouble
        cout << "master: " << master_color_image.get_device_timestamp().count() << " ";
        if (master_capture.get_depth_image())
        {
            cout << master_capture.get_depth_image().get_device_timestamp().count() - last_master_depth.count() << " ";
            last_master_depth = master_capture.get_depth_image().get_device_timestamp();
        }
        else
        {
            cout << "----------"
                 << " ";
        }
        cout << "sub: ";
        if (sub_capture.get_color_image())
        {
            cout << sub_capture.get_color_image().get_device_timestamp().count() << " ";
        }
        else
        {
            cout << "----------"
                 << " ";
        }
        if (sub_capture.get_depth_image())
        {
            cout << sub_capture.get_depth_image().get_device_timestamp().count() - last_sub_depth.count() << "\n";
            last_sub_depth = sub_capture.get_depth_image().get_device_timestamp();
        }
        else
        {
            cout << "----------"
                 << "\n";
        }

        if (master_color_image && sub_image)
        {
            std::chrono::microseconds sub_image_time = sub_image.get_device_timestamp();
            std::chrono::microseconds master_color_image_time = master_color_image.get_device_timestamp();
            // The subordinate's color image timestamp, ideally, is the master's color image timestamp plus the
            // delay we configured between the master device color camera and subordinate device color camera
            std::chrono::microseconds expected_sub_image_time = master_color_image_time + std::chrono::microseconds{
                sub_config.subordinate_delay_off_master_usec
            };
            std::chrono::microseconds sub_image_time_error = sub_image_time - expected_sub_image_time;
            // The time error's absolute value must be within the permissible range. So, for example, if
            // MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP is 2, offsets of -2, -1, 0, 1, and -2 are
            // permitted
            // cout << sub_image_time_error.count() << "\n";
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
                sub.get_capture(&sub_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
            }
            else if (sub_image_time_error > MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP)
            {
                // Example, where MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP is 1
                // time                    t=1  t=2  t=3
                // actual timestamp        .    .    x
                // expected timestamp      x    .    .
                // error: 3 - 1 = 2, which is more than the worst-case-allowable offset of 1
                // the subordinate camera image timestamp was later than it is allowed to be. This means the
                // subordinate is ahead and we need to update the master to get the master caught up
                cout << "Master lagging...\n";
                master.get_capture(&master_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
            }
            else
            {
                // The captures are sufficiently synchronized. Exit the function.
                cout << "Got synchronized images!\n";
                have_synced_images = true;
            }
        }
        else
        {
            // One of the captures or one of the images are bad, so just replace both. One could make this more
            // sophisticated and try to only to only replace one of these captures, et cetera, to try to keep a good one
            // but we'll keep things simple and just throw both away and try again.
            cout << "One of the images was bad!\n";
            master.get_capture(&master_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
            sub.get_capture(&sub_capture, std::chrono::milliseconds{ K4A_WAIT_INFINITE });
        }
    }
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
        // Likewise, if the chessboard was found in the subordinate image, it was not found in the
        // master image.
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
    // Before we go on, there's a quick problem with calibration to address.
    // Because the chessboard looks the same when rotated 180 degrees, it is possible that the
    // chessboard corner finder may find the correct points, but in the wrong order.

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

    // The problem occurs when this case happens: the find_chessboard() function correctly
    // identifies the points on the checkerboard (shown as 'x's) but the order of those points
    // differs between images taken by the two cameras. Specifically, the first point in the list
    // of points found for the first image (1) is the *last* point in the list of points found for
    // the second image (2), though they correspond to the same physical point on the chessboard.

    // To avoid this problem, we can make the assumption that both of the cameras will be oriented
    // in a similar manner (e.g. turning one of the cameras upside down will break this assumption)
    // and enforce that the vector between the first and last points found in pixel space (which
    // will be at opposite ends of the chessboard) are pointing the same direction- so, the dot
    // product of the two vectors is positive.

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

void stereo_calibration(const k4a::calibration &master_calib,
                        const k4a::calibration &sub_calib,
                        const vector<cv::Point2f> &master_chessboard_corners,
                        const vector<cv::Point2f> &sub_chessboard_corners,
                        const cv::Size &image_size,
                        cv::Mat &R,
                        cv::Vec3d &t)
{
    // We have points in each image that correspond to the corners that the findChessboardCorners
    // function found. However, we still need the points in 3 dimensions that these points
    // correspond to. Because we are ultimately only interested in find a transformation between two
    // cameras, these points don't have to correspond to an external "origin" point. The only
    // important thing is that the relative distances between points are accurate. As a result, we
    // can simply make the first corresponding point (0, 0) and construct the remaining points based
    // on that one. The order of points inserted into the vector here matches the ordering of
    // findChessboardCorners. The units of these points are in millimeters, mostly because the depth
    // provided by the depth cameras is also provided in millimeters, which makes for easy comparison.
    vector<cv::Point3f> chessboard_corners_world;
    for (size_t w = 0; w < board_width; ++w)
    {
        for (size_t h = 0; h < board_height; ++h)
        {
            chessboard_corners_world.emplace_back(cv::Point3f{ h * board_square_size, w * board_square_size, 0.0 });
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
    // * note: OpenCV's stereoCalibrate actually requires as input an array of arrays of points for
    // these arguments, allowing a caller to provide multiple frames from the same camera with
    // corresponding points. For example, if extremely high precision was required, many images
    // could be taken with each camera, and findChessboardCorners applied to each of those images,
    // and OpenCV can jointly solve for all of the pairs of corresponding images. However, to keep
    // things simple, we use only one image from each device to calibrate.
    // This is also why each of the vectors of corners is placed into another vector.
    // A function in OpenCV's calibration function also requires that these points be F32 types, so
    // we use those. However, OpenCV still provides doubles as output, strangely enough.
    vector<vector<cv::Point3f>> chessboard_corners_world_nested_for_cv(1, chessboard_corners_world);
    vector<vector<cv::Point2f>> master_corners_nested_for_cv(1, master_chessboard_corners);
    vector<vector<cv::Point2f>> sub_corners_nested_for_cv(1, sub_chessboard_corners);

    cv::Mat master_camera_matrix = k4a_calibration_to_color_camera_matrix(master_calib);
    cv::Mat sub_camera_matrix = k4a_calibration_to_color_camera_matrix(sub_calib);
    vector<float> master_dist_coeff = k4a_calibration_to_color_camera_dist_coeffs(master_calib);
    vector<float> sub_dist_coeff = k4a_calibration_to_color_camera_dist_coeffs(sub_calib);

    // Finally, we'll actually calibrate the cameras.
    // Pass subordinate first, then master, because we want a transform from subordinate to master.
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
                                       cv::CALIB_FIX_INTRINSIC | cv::CALIB_RATIONAL_MODEL);
    cout << "Finished calibrating!\n";
    cout << "Got error of " << error << "\n";
}

int main()
{
    try
    {
        // TODO find a home for this comment
        // Note that many sections of the code avoid using std::endl in favor of using \n, because sections of code
        // dedicated to interacting with both cameras can be sensitive to timing and flushing the buffer takes time.

        // TODO is this comment necessary anymore?
        // For calibration, we require an image containing the chessboard taken at the same time in both
        // the master and the subordinate perspectives. Because we currently have synchronized images,
        // we need to collect new captures from both devices: collecting only from one device would mean
        // the images are not synchronized. However, at this point, simply getting a capture from both
        // cameras does not guarantee that those captures are synchronized, even though they are
        // acquired by calling get_capture on both devices immediately after having two synchronized
        // images, because either device could drop a frame at any point. So, we will get two new
        // images, finish up this loop iteration, then return to the beginning of the loop, which will
        // keep running until two synchronized images are found and they are searched for the
        // chessboard. At least one of the images did not contain the board. So, if the chessboard was
        // found in the master image, it was not found in the subordinate image.

        // Require at least 2 cameras
        const size_t num_devices = k4a::device::get_installed_count();
        if (num_devices < 2)
        {
            throw std::runtime_error("At least 2 cameras are required!");
        }
        // We will use devices[0] as the master device
        vector<k4a::device> devices;
        devices.emplace_back(k4a::device::open(0));
        devices.emplace_back(k4a::device::open(1));
        // Configure both of the cameras with the same framerate, resolution, exposure, and firmware
        // NOTE: Both cameras must have the same configuration (TODO) what exactly needs to be the same
        k4a_device_configuration_t camera_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        camera_config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        camera_config.color_resolution = K4A_COLOR_RESOLUTION_720P; // TODO none after calib
        camera_config.depth_mode = K4A_DEPTH_MODE_WFOV_UNBINNED;
        camera_config.camera_fps = K4A_FRAMES_PER_SECOND_15;
        camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
        camera_config.subordinate_delay_off_master_usec = 160; // Allowing at least 160 microseconds between depth
                                                               // cameras should ensure they do not interfere with one
                                                               // another. Perhaps there should be a nod to how to
                                                               // handle series of delays
        camera_config.synchronized_images_only = true;         // only needs to be true for the master
        vector<k4a_device_configuration_t> configurations(num_devices, camera_config);
        // special case: the first config needs to be set to be the master
        configurations.front().wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
        configurations.front().subordinate_delay_off_master_usec = 0;
        configurations.front().color_resolution = K4A_COLOR_RESOLUTION_720P;
        configurations.front().synchronized_images_only = true;

        devices[0].set_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, K4A_COLOR_CONTROL_MODE_MANUAL, 8000);
        devices[1].set_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, K4A_COLOR_CONTROL_MODE_MANUAL, 8000);
        devices[0].set_color_control(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, K4A_COLOR_CONTROL_MODE_MANUAL, 2);
        devices[1].set_color_control(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, K4A_COLOR_CONTROL_MODE_MANUAL, 2);

        vector<k4a::calibration> calibrations;
        for (size_t i = 0; i < devices.size(); ++i)
        {
            calibrations.emplace_back(
                devices[i].get_calibration(configurations[i].depth_mode, configurations[i].color_resolution));
        }

        // make sure that the master has sync out connected (ready to send commands)
        // TODO this should just detect instead
        if (!devices[0].is_sync_out_connected())
        {
            throw std::runtime_error("Out sync needs to be connected to master camera!");
        }
        // make sure that all other devices are have sync in connected (ready to receive commands from master)
        for (auto i = devices.cbegin() + 1; i != devices.cend(); ++i)
        {
            if (!i->is_sync_in_connected())
            {
                throw std::runtime_error("Out sync needs to be connected to master camera!");
            }
        }

        // now it's time to start the cameras. All subordinate cameras must be started before the master
        for (size_t i = 1; i < devices.size(); ++i)
        {
            devices[i].start_cameras(&configurations[i]);
        }
        devices[0].start_cameras(&configurations[0]);

        // These will hold the rotation and translation we get from calibration
        cv::Mat R_color_sub_to_color_master;
        cv::Vec3d t_color_sub_to_color_master;
        cv::Mat R_depth_sub_to_color_sub;
        cv::Vec3d t_depth_sub_to_color_sub;

        // These will hold the rotation and translation we get by composing internal and calibrated transforms
        cv::Mat R_depth_sub_to_color_master;
        cv::Vec3d t_depth_sub_to_color_master;

        const k4a::calibration &sub_calib = devices.back().get_calibration(configurations.back().depth_mode,
                                                                           configurations.back().color_resolution);
        bool calibrated = false;
        while (!calibrated)
        {
            k4a::capture master_capture, sub_capture;
            get_synchronized_captures(devices[0], devices[1], configurations[1], master_capture, sub_capture);
            // get_color_image is guaranteed to work because get_synchronized_captures, used as above, checks to ensure
            // that color images are present on both captures
            k4a::image master_color_image = master_capture.get_color_image();
            k4a::image sub_color_image = sub_capture.get_color_image();
            // Differentiated from the k4a::image master_color_image with 'cv_' prefix
            cv::Mat cv_master_color_image = k4a_color_to_opencv(master_color_image);
            cv::Mat cv_sub_color_image = k4a_color_to_opencv(sub_color_image);
            cv::Size chessboard_pattern(board_height, board_width);
            vector<cv::Point2f> master_chessboard_corners;
            vector<cv::Point2f> sub_chessboard_corners;

            bool ready_to_calibrate = find_chessboard_corners_helper(cv_master_color_image,
                                                                     cv_sub_color_image,
                                                                     chessboard_pattern,
                                                                     master_chessboard_corners,
                                                                     sub_chessboard_corners);

            if (ready_to_calibrate)
            {
                stereo_calibration(calibrations[0],
                                   calibrations[1],
                                   master_chessboard_corners,
                                   sub_chessboard_corners,
                                   cv_master_color_image.size(),
                                   R_color_sub_to_color_master,
                                   t_color_sub_to_color_master);
                calibrated = true;
            }
        }

        // Next, we'll want to extend this a bit. By calibrating, we have a transformation from the subordinate color
        // camera perspective to the master color camera perspective. We have the transformation from subordinate depth
        // to subordinate color (it's stored in extrinsics in the calibration type and we'll get it with
        // k4a_calibration_to_depth_to_color_R_t(). Next, we will combine these transformations to create a
        // transformation from subordinate depth to master color.
        k4a_calibration_to_depth_to_color_R_t(sub_calib, R_depth_sub_to_color_sub, t_depth_sub_to_color_sub);
        cout << "Matrices obtained from depth camera" << endl;
        cout << R_depth_sub_to_color_sub << endl;
        cout << t_depth_sub_to_color_sub << endl;

        // next, we're going to construct a homogeneous equivalent
        cout << "Construct homog depth sub to color sub" << endl;
        cv::Mat depth_sub_to_color_sub_h = construct_homogeneous(R_depth_sub_to_color_sub, t_depth_sub_to_color_sub);
        cout << depth_sub_to_color_sub_h << endl;
        cout << "Construct homog color sub to color master" << endl;
        cout << R_color_sub_to_color_master << endl;
        cout << t_color_sub_to_color_master << endl;
        cv::Mat color_sub_to_color_master_h = construct_homogeneous(R_color_sub_to_color_master,
                                                                    t_color_sub_to_color_master);
        cout << color_sub_to_color_master_h << endl;
        cout << "Multiply" << endl;
        cv::Mat depth_sub_to_color_master_h = depth_sub_to_color_sub_h * color_sub_to_color_master_h;
        deconstruct_homogeneous(depth_sub_to_color_master_h, R_depth_sub_to_color_master, t_depth_sub_to_color_master);
        cout << "Homogeneous transform depth sub to color master:" << endl;
        cout << depth_sub_to_color_master_h << endl;
        cout << "Depth sub to color master R:" << endl;
        cout << R_depth_sub_to_color_master << endl;
        cout << "Depth sub to color master t:" << endl;
        cout << t_depth_sub_to_color_master << endl;

        // Now, we're going to set up the transformations. DO THIS OUTSIDE OF YOUR MAIN LOOP!
        // Constructing transformations does a lot of preemptive work to make the transform as fast as possible.
        k4a::transformation master_depth_to_master_color(calibrations[0]);
        k4a::transformation sub_depth_to_sub_color(calibrations[1]);

        // Now it's time to get clever. We're going to update the existing calibration extrinsics on getting from
        // the sub depth camera to the sub color camera, overwriting it with the transformation to get from the sub
        // depth camera to the master color camera
        set_k4a_calibration_depth_to_color_from_R_t(calibrations[1],
                                                    R_depth_sub_to_color_master,
                                                    t_depth_sub_to_color_master);
        k4a::transformation sub_depth_to_master_color(calibrations[1]);

        while (true)
        {
            vector<k4a::capture> device_captures(devices.size());
            get_synchronized_captures(devices[0],
                                      devices[1],
                                      configurations[1],
                                      device_captures[0],
                                      device_captures[1],
                                      true);

            k4a::image master_color_image = device_captures[0].get_color_image();
            // get depth images
            vector<k4a::image> depth_images;
            depth_images.reserve(devices.size());
            for (const k4a::capture &cap : device_captures)
            {
                depth_images.emplace_back(cap.get_depth_image());
            }

            // if we reach this point, we know that we're good to go.
            // first, let's check out the timestamps. TODO
            // for (size_t i = 0; i < depth_images.size(); ++i)
            // {
            //     cout << "Color image timestamp: " << i << " " << color_images[i].get_device_timestamp().count() <<
            //     "\n"; cout << "Depth image timestamp: " << i << " " << depth_images[i].get_device_timestamp().count()
            //     << "\n";
            // }

            // let's greenscreen out things that are far away.
            // first: let's get the master depth image into the color camera space
            // create a copy with the same parameters
            k4a::image k4a_master_depth_in_master_color = k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16,
                                                                             master_color_image.get_width_pixels(),
                                                                             master_color_image.get_height_pixels(),
                                                                             master_color_image.get_width_pixels() *
                                                                                 static_cast<int>(sizeof(uint16_t)));

            master_depth_to_master_color.depth_image_to_color_camera(depth_images[0],
                                                                     &k4a_master_depth_in_master_color);

            // now, create an OpenCV version of the depth matrix for easy usage
            cv::Mat opencv_master_depth_in_master_color = k4a_depth_to_opencv(k4a_master_depth_in_master_color);

            // now let's get the subordinate depth image into the color camera space
            k4a::image k4a_sub_depth_in_sub_color = k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16,
                                                                       master_color_image.get_width_pixels(),
                                                                       master_color_image.get_height_pixels(),
                                                                       master_color_image.get_width_pixels() *
                                                                           static_cast<int>(sizeof(uint16_t)));
            sub_depth_to_sub_color.depth_image_to_color_camera(depth_images[1], &k4a_sub_depth_in_sub_color);
            cv::Mat opencv_sub_depth_in_sub_color = k4a_depth_to_opencv(k4a_sub_depth_in_sub_color);

            // Finally, it's time to create the opencv image for the depth image in the master color perspective
            k4a::image k4a_sub_depth_in_master_color = k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16,
                                                                          master_color_image.get_width_pixels(),
                                                                          master_color_image.get_height_pixels(),
                                                                          master_color_image.get_width_pixels() *
                                                                              static_cast<int>(sizeof(uint16_t)));
            sub_depth_to_master_color.depth_image_to_color_camera(depth_images[1], &k4a_sub_depth_in_master_color);
            cv::Mat opencv_sub_depth_in_master_color = k4a_depth_to_opencv(k4a_sub_depth_in_master_color);

            // cv::Mat normalized_opencv_sub_depth_in_sub_color;
            // cv::Mat normalized_opencv_master_depth_in_master_color;
            cv::Mat normalized_opencv_sub_depth_in_master_color;
            // cv::normalize(opencv_sub_depth_in_sub_color,
            //               normalized_opencv_sub_depth_in_sub_color,
            //               0,
            //               256,
            //               cv::NORM_MINMAX);
            // cv::normalize(opencv_master_depth_in_master_color,
            //               normalized_opencv_master_depth_in_master_color,
            //               0,
            //               256,
            //               cv::NORM_MINMAX);
            cv::normalize(opencv_sub_depth_in_master_color,
                          normalized_opencv_sub_depth_in_master_color,
                          0,
                          256,
                          cv::NORM_MINMAX);
            // cv::Mat grayscale_opencv_sub_depth_in_sub_color;
            // cv::Mat grayscale_opencv_master_depth_in_master_color;
            cv::Mat grayscale_opencv_sub_depth_in_master_color;
            // normalized_opencv_sub_depth_in_sub_color.convertTo(grayscale_opencv_sub_depth_in_sub_color, CV_32FC3);
            // normalized_opencv_master_depth_in_master_color.convertTo(grayscale_opencv_master_depth_in_master_color,
            //                                                          CV_32FC3);
            normalized_opencv_sub_depth_in_master_color.convertTo(grayscale_opencv_sub_depth_in_master_color, CV_32FC3);
            // cv::imshow("Master depth in master color", grayscale_opencv_master_depth_in_master_color);
            // cv::waitKey(500);
            // cv::imshow("Subordinate depth in subordinate color", grayscale_opencv_sub_depth_in_sub_color);
            // cv::waitKey(500);
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
            // cout << opencv_sub_depth_in_master_color << endl;
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
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
