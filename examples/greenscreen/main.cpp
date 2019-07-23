#include <algorithm>
#include <iostream>
#include <vector>
#include <string>

#include <k4a/k4a.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using std::cout;
using std::endl;
using std::vector;

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
                   CV_16UC1,
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

int main()
{
    try
    {
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
        // NOTE: Both cameras must have the same configuration (TODO) what exactly needs to
        k4a_device_configuration_t camera_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        camera_config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        camera_config.color_resolution = K4A_COLOR_RESOLUTION_720P; // TODO none after calib
        camera_config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
        camera_config.camera_fps = K4A_FRAMES_PER_SECOND_15;
        camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE; // TODO SUBORDINATE
        camera_config.subordinate_delay_off_master_usec = 0; // TODO nope this should be the min, sort through all
                                                             // the syncing
        camera_config.synchronized_images_only = false;
        vector<k4a_device_configuration_t> configurations(num_devices, camera_config);
        // special case: the first config needs to be set to be the master
        configurations.front().wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
        configurations.front().subordinate_delay_off_master_usec = 0; // TODO delay for the rest
        configurations.front().color_resolution = K4A_COLOR_RESOLUTION_720P;
        configurations.front().synchronized_images_only = true;

        vector<k4a::calibration> calibrations;
        // fill the calibrations vector
        for (size_t i = 0; i < devices.size(); ++i)
        {
            calibrations.emplace_back(
                devices[i].get_calibration(configurations[i].depth_mode, configurations[i].color_resolution));
        }

        // make sure that the master has sync out connected (ready to send commands)
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
        cv::Mat R_color_sub_to_color_master, E, F;
        cv::Vec3d t_color_sub_to_color_master;
        cv::Mat R_depth_sub_to_color_sub;
        cv::Vec3d t_depth_sub_to_color_sub;

        // These will hold the rotation and translation we get by composing internal and calibrated transforms
        cv::Mat R_depth_sub_to_color_master;
        cv::Vec3d t_depth_sub_to_color_master;

        // the cameras have been started. Calibration time!
        while (true)
        {
            vector<k4a::capture> device_captures(devices.size());
            bool got_captures = true;
            for (size_t i = 0; i < devices.size(); ++i)
            {
                if (!devices[i].get_capture(&device_captures[i], std::chrono::milliseconds{ K4A_WAIT_INFINITE }) ||
                    !device_captures[i] || !device_captures[i].get_color_image())
                {
                    got_captures = false;
                }
            }

            if (!got_captures)
            {
                cout << "Didn't get the capture and depth image...trying again\n"; // TODO depth?
                continue;
            }

            vector<cv::Mat> color_images;
            color_images.reserve(devices.size());
            for (const k4a::capture &cap : device_captures)
            {
                color_images.emplace_back(k4a_color_to_opencv(cap.get_color_image()));
            }

            // depths are present on both images. Time to calibrate.
            size_t board_height = 8;
            size_t board_width = 6;
            float board_square_size = 33.; // mine was 33 millimeters TODO generalize
            cv::Size chessboard_pattern(board_height, board_width);

            // indexing: first by camera, then by frame, then by which corner
            // TODO add more frames?
            vector<vector<cv::Point2d>> one_initialized_frame(1);
            vector<vector<vector<cv::Point2d>>> corners(devices.size(), one_initialized_frame);
            bool found_chessboard = true;
            for (size_t i = 0; i < color_images.size(); ++i)
            {
                found_chessboard = found_chessboard &&
                                   cv::findChessboardCorners(color_images[i],
                                                             chessboard_pattern,
                                                             corners[i][0],
                                                             cv::CALIB_CB_ADAPTIVE_THRESH); // TODO
                                                                                            // CV_CALIB_CB_FILTER_QUADS
                found_chessboard = found_chessboard && !corners[i][0].empty();
                // TODO get subpixel working
                // if (found_chessboard)
                // {
                //     // Term criteria was taken from OpenCV's website
                //     cv::cornerSubPix(color_images[i],
                //                      corners[i],
                //                      chessboard_pattern,
                //                      cv::Size(-1, -1),
                //                      cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 30, 0.1));
                // }
            }

            if (!found_chessboard)
            {
                cout << "Couldn't find the chessboard corners...trying again\n";
                continue;
            }

            // TODO print out seeing the image
            for (size_t i = 0; i < color_images.size(); ++i)
            {
                // For some bizarre reason, drawChessboardCorners doesn't like doubles.
                vector<cv::Point2f> corners_float;
                corners_float.reserve(corners[i][0].size());
                for (const cv::Point2d &p : corners[i][0])
                {
                    corners_float.emplace_back(cv::Point2f{ static_cast<float>(p.x), static_cast<float>(p.y) });
                }
                cv::drawChessboardCorners(color_images[i], chessboard_pattern, corners_float, found_chessboard);
                std::string title = std::string("Chessboard view from camera ") + std::to_string(i);
                cv::imshow(title, color_images[i]);
                // cv::setMouseCallback(title, onMouse);
                cv::waitKey(500);
            }

            // now construct the points needed to calibrate
            // indexed by frame, then by point index
            // TODO support multiple frames
            vector<vector<cv::Point3d>> chessboard_corners_3d(1); // start with 1 frame
            // the points exist on a plane
            cout << "Filling plane details!" << std::endl;
            for (size_t w = 0; w < board_width; ++w)
            {
                for (size_t h = 0; h < board_height; ++h)
                {
                    chessboard_corners_3d[0].emplace_back(
                        cv::Point3d{ h * board_square_size, w * board_square_size, 0.0 });
                }
            }

            // this whole next section is only necessary because collectCalibrationData in OpenCV doesn't work with
            // doubles
            vector<vector<cv::Point3f>> chessboard_corners_3d_float(1); // ugh opencv
            for (const cv::Point3d &p : chessboard_corners_3d[0])
            {
                chessboard_corners_3d_float[0].emplace_back(
                    cv::Point3f{ static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z) });
            }
            vector<vector<vector<cv::Point2f>>> corners_float; // ugh opencv
            for (const vector<vector<cv::Point2d>> &p_vec_vec : corners)
            {
                corners_float.emplace_back();        // make a vector for this camera's corners
                corners_float.back().emplace_back(); // make a vector for this camera's corners with 1 frame
                for (const cv::Point2d &p : p_vec_vec[0])
                {
                    corners_float.back()[0].emplace_back(
                        cv::Point2f{ static_cast<float>(p.x), static_cast<float>(p.y) });
                }
            }

            // We need camera matrices and distortion coefficients for each of the devices

            // get a rotation and translation between the master and subordinate camera
            cout << "Getting references!" << std::endl;
            const vector<vector<cv::Point2f>> &master_corners_2d = corners_float.front();
            const vector<vector<cv::Point2f>> &subordinate_corners_2d = corners_float.back();
            const k4a::calibration &master_calib =
                devices.front().get_calibration(configurations.front().depth_mode,
                                                configurations.front().color_resolution);
            const k4a::calibration &sub_calib = devices.back().get_calibration(configurations.back().depth_mode,
                                                                               configurations.back().color_resolution);
            cout << "Getting camera matrices!" << std::endl;
            cv::Mat master_camera_matrix = k4a_calibration_to_color_camera_matrix(master_calib);
            cv::Mat sub_camera_matrix = k4a_calibration_to_color_camera_matrix(sub_calib);
            cout << "Getting camera dist coefficients!" << std::endl;
            vector<float> master_dist_coeff = k4a_calibration_to_color_camera_dist_coeffs(master_calib);
            vector<float> sub_dist_coeff = k4a_calibration_to_color_camera_dist_coeffs(sub_calib);

            // TODO depth, not color!

            cout << "Preparing to calibrate!" << std::endl;
            for (size_t i = 0; i < master_corners_2d.front().size(); ++i)
            {
                cout << "Master point: ";
                cout << master_corners_2d.front()[i] << endl;
                cout << "Subordinate point: ";
                cout << subordinate_corners_2d.front()[i] << endl;
                cout << "3D point: ";
                cout << chessboard_corners_3d_float.front()[i] << endl;
            }
            // cout << subordinate_corners_2d << std::endl;
            // cout << master_corners_2d << std::endl;
            // TODO is this ok actually
            sub_camera_matrix.convertTo(sub_camera_matrix, CV_32F);
            master_camera_matrix.convertTo(master_camera_matrix, CV_32F);
            cout << "Camera matrices!" << endl;
            cout << sub_camera_matrix << endl;
            cout << master_camera_matrix << endl;
            for (float f : sub_dist_coeff)
            {
                cout << f << " ";
            }
            cout << endl;
            for (float f : master_dist_coeff)
            {
                cout << f << " ";
            }
            cout << endl;
            cout << "Types" << endl;
            cout << CV_32F << endl;
            cout << sub_camera_matrix.type() << endl;
            cout << master_camera_matrix.type() << endl;
            double error = cv::stereoCalibrate(chessboard_corners_3d_float,
                                               subordinate_corners_2d,
                                               master_corners_2d,
                                               sub_camera_matrix,
                                               sub_dist_coeff,
                                               master_camera_matrix,
                                               master_dist_coeff,
                                               color_images[0].size(),
                                               R_color_sub_to_color_master,
                                               t_color_sub_to_color_master,
                                               E,
                                               F,
                                               cv::CALIB_FIX_INTRINSIC | cv::CALIB_RATIONAL_MODEL);
            cout << "Finished calibrating!" << std::endl;
            cout << "Got error of " << error << endl;

            cout << R_color_sub_to_color_master << std::endl;
            cout << t_color_sub_to_color_master << std::endl;

            // the goal: the extrinsics already have the rotation + translation for depth_sub -> color_sub
            // and we have depth_color -> camera_color so let's combine them
            k4a_calibration_to_depth_to_color_R_t(sub_calib, R_depth_sub_to_color_sub, t_depth_sub_to_color_sub);
            cout << "Matrices obtained from camera" << endl;
            cout << R_depth_sub_to_color_sub << endl;
            cout << t_depth_sub_to_color_sub << endl;

            // next, we're going to construct a homogeneous equivalent
            cout << "Construct homog depth sub to color sub" << endl;
            cv::Mat depth_sub_to_color_sub_h = construct_homogeneous(R_depth_sub_to_color_sub,
                                                                     t_depth_sub_to_color_sub);
            cout << depth_sub_to_color_sub_h << endl;
            cout << "Construct homog color sub to color master" << endl;
            cv::Mat color_sub_to_color_master_h = construct_homogeneous(R_color_sub_to_color_master,
                                                                        t_color_sub_to_color_master);
            cout << color_sub_to_color_master_h << endl;
            cout << "Multiply" << endl;
            cv::Mat depth_sub_to_color_master_h = depth_sub_to_color_sub_h * color_sub_to_color_master_h;
            deconstruct_homogeneous(depth_sub_to_color_master_h,
                                    R_depth_sub_to_color_master,
                                    t_depth_sub_to_color_master);
            cout << "Homogeneous transform depth sub to color master:" << endl;
            cout << depth_sub_to_color_master_h << endl;
            cout << "Depth sub to color master R:" << endl;
            cout << R_depth_sub_to_color_master << endl;
            cout << "Depth sub to color master t:" << endl;
            cout << t_depth_sub_to_color_master << endl;

            break;
        }

        // Now let's get the images we need.

        while (true)
        {
            vector<k4a::capture> device_captures(devices.size());
            // first, go through and get a capture from each
            for (size_t i = 0; i < devices.size(); ++i)
            {
                if (!devices[i].get_capture(&device_captures[i], std::chrono::milliseconds{ K4A_WAIT_INFINITE }))
                {
                    throw std::runtime_error("Getting a capture failed!");
                }
            }

            // make sure all of the captures are valid
            if (!std::all_of(device_captures.cbegin(), device_captures.cbegin(), [](const k4a::capture &c) {
                    return c;
                }))
            {
                cout << "Not all device captures were valid!\n";
                continue;
            }

            k4a::image master_color_image = device_captures[0].get_color_image();
            if (!master_color_image)
            {
                cout << "Master doesn't have a color image!\n";
                continue;
            }

            // get depth images
            vector<k4a::image> depth_images;
            depth_images.reserve(devices.size());
            for (const k4a::capture &cap : device_captures)
            {
                depth_images.emplace_back(cap.get_depth_image());
            }
            if (!std::all_of(depth_images.cbegin(), depth_images.cend(), [](const k4a::image &i) { return i; }))
            {
                cout << "One or more invalid depth images!\n"; //  << std::endl;
                continue;
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

            // now fill it with the shifted version
            // to do so, we need the transformation from TODO
            k4a::transformation master_depth_to_master_color(calibrations[0]);
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
            k4a::transformation sub_depth_to_sub_color(calibrations[1]);
            sub_depth_to_sub_color.depth_image_to_color_camera(depth_images[1], &k4a_sub_depth_in_sub_color);
            cv::Mat opencv_sub_depth_in_sub_color = k4a_depth_to_opencv(k4a_sub_depth_in_sub_color);

            // Finally, it's time to create the opencv image for the depth image in the master color perspective
            k4a::image k4a_sub_depth_in_master_color = k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16,
                                                                          master_color_image.get_width_pixels(),
                                                                          master_color_image.get_height_pixels(),
                                                                          master_color_image.get_width_pixels() *
                                                                              static_cast<int>(sizeof(uint16_t)));
            // Now it's time to get clever. We're going to update the existing calibration extrinsics on getting from
            // the sub depth camera to the sub color camera, overwriting it with the transformation to get from the sub
            // depth camera to the master color camera
            set_k4a_calibration_depth_to_color_from_R_t(calibrations[1],
                                                        R_depth_sub_to_color_master,
                                                        t_depth_sub_to_color_master);
            k4a::transformation sub_depth_to_master_color(calibrations[1]);
            sub_depth_to_master_color.depth_image_to_color_camera(depth_images[1], &k4a_sub_depth_in_master_color);
            cv::Mat opencv_sub_depth_in_master_color = k4a_depth_to_opencv(k4a_sub_depth_in_master_color);

            cv::Mat normalized_opencv_sub_depth_in_sub_color;
            cv::Mat normalized_opencv_master_depth_in_master_color;
            cv::Mat normalized_opencv_sub_depth_in_master_color;
            cv::normalize(opencv_sub_depth_in_sub_color,
                          normalized_opencv_sub_depth_in_sub_color,
                          0,
                          256,
                          cv::NORM_MINMAX);
            cv::normalize(opencv_master_depth_in_master_color,
                          normalized_opencv_master_depth_in_master_color,
                          0,
                          256,
                          cv::NORM_MINMAX);
            cv::normalize(opencv_sub_depth_in_master_color,
                          normalized_opencv_sub_depth_in_master_color,
                          0,
                          256,
                          cv::NORM_MINMAX);
            cv::Mat grayscale_opencv_sub_depth_in_sub_color;
            cv::Mat grayscale_opencv_master_depth_in_master_color;
            cv::Mat grayscale_opencv_sub_depth_in_master_color;
            normalized_opencv_sub_depth_in_sub_color.convertTo(grayscale_opencv_sub_depth_in_sub_color, CV_32FC3);
            normalized_opencv_master_depth_in_master_color.convertTo(grayscale_opencv_master_depth_in_master_color,
                                                                     CV_32FC3);
            normalized_opencv_sub_depth_in_master_color.convertTo(grayscale_opencv_sub_depth_in_master_color, CV_32FC3);
            cv::imshow("Master depth in master color", grayscale_opencv_master_depth_in_master_color);
            cv::waitKey(500);
            cv::imshow("Subordinate depth in subordinate color", grayscale_opencv_sub_depth_in_sub_color);
            cv::waitKey(500);
            cv::imshow("Subordinate depth in master color", grayscale_opencv_sub_depth_in_master_color);
            cv::waitKey(500);

            cv::Mat master_opencv_color_image = k4a_color_to_opencv(master_color_image);

            // create the image that will be be used as output
            cv::Mat output_image(master_opencv_color_image.rows,
                                 master_opencv_color_image.cols,
                                 CV_32FC3,
                                 cv::Scalar(0, 0, 0));

            // this will be added
            cv::Mat green(master_opencv_color_image.rows,
                          master_opencv_color_image.cols,
                          CV_32FC3,
                          cv::Scalar(0, .25, 0));

            const uint16_t THRESHOLD = 1000; // TODO
            cv::Mat combined_depth = (grayscale_opencv_master_depth_in_master_color +
                                      grayscale_opencv_sub_depth_in_master_color) /
                                     2;

            cv::imshow("combined", combined_depth);
            cv::waitKey(0);
            cout << "Test1" << endl;
            cv::Mat master_image_float;
            master_opencv_color_image.convertTo(master_image_float, CV_32F);
            master_image_float = master_image_float / 255.0;
            cv::Mat greened_matrix = master_image_float + cv::Scalar(0, .25, 0);
            cv::waitKey(0);
            cout << "Test2" << endl;
            cv::Mat mask;
            cout << "Test3" << endl;
            cv::bitwise_and(combined_depth < THRESHOLD, combined_depth != 0, mask);
            cout << "Test4" << endl;
            cv::Mat inverse_mask;
            cv::bitwise_not(mask, inverse_mask);
            cout << "Test8" << endl;
            cout << mask << endl;
            master_image_float.copyTo(output_image, mask);
            greened_matrix.copyTo(output_image, inverse_mask);
            // please?
            cv::imshow("Greenscreened", output_image);
            cv::waitKey(0);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
