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

cv::Mat k4a_calibration_to_depth_camera_matrix(const k4a::calibration &cal)
{
    const k4a_calibration_intrinsic_parameters_t::_param &i = cal.depth_camera_calibration.intrinsics.parameters.param;
    cv::Mat camera_matrix = cv::Mat::zeros(3, 3, CV_64F);
    camera_matrix.at<double>(0, 0) = i.fx;
    camera_matrix.at<double>(1, 1) = i.fx;
    camera_matrix.at<double>(0, 2) = i.cx;
    camera_matrix.at<double>(1, 2) = i.cy;
    camera_matrix.at<double>(2, 2) = 1;
    return camera_matrix;
}

vector<double> k4a_calibration_to_depth_camera_dist_coeffs(const k4a::calibration &cal)
{
    const k4a_calibration_intrinsic_parameters_t::_param &i = cal.depth_camera_calibration.intrinsics.parameters.param;
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

        // These will hold the rotation and translation we want
        cv::Mat R;
        cv::Vec3d T;

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
            double board_square_size = .033; // mine was 33 millimeters TODO generalize
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
                cv::imshow(std::string("Chessboard view from camera ") + std::to_string(i), color_images[i]);
                cv::waitKey(0);
            }

            // now construct the points needed to calibrate
            // indexed by frame, then by point index
            // TODO support multiple frames
            vector<vector<cv::Point3d>> chessboard_corners_3d(1); // start with 1 frame
            // the points exist on a plane
            cout << "Filling plane details!" << std::endl;
            for (size_t h = 0; h < board_height; ++h)
            {
                for (size_t w = 0; w < board_width; ++w)
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
            cv::Mat master_camera_matrix = k4a_calibration_to_depth_camera_matrix(master_calib);
            cv::Mat sub_camera_matrix = k4a_calibration_to_depth_camera_matrix(sub_calib);
            cout << "Getting camera dist coefficients!" << std::endl;
            vector<double> master_dist_coeff = k4a_calibration_to_depth_camera_dist_coeffs(master_calib);
            vector<double> sub_dist_coeff = k4a_calibration_to_depth_camera_dist_coeffs(sub_calib);

            // TODO depth, not color!

            cv::Mat E, F;

            cout << master_camera_matrix << std::endl;
            cout << sub_camera_matrix << std::endl;
            cout << "Preparing to calibrate!" << std::endl;
            cv::stereoCalibrate(chessboard_corners_3d_float,
                                master_corners_2d,
                                subordinate_corners_2d,
                                master_camera_matrix,
                                master_dist_coeff,
                                sub_camera_matrix,
                                sub_dist_coeff,
                                color_images[0].size(),
                                R,
                                T,
                                E,
                                F,
                                cv::CALIB_FIX_INTRINSIC);
            cout << "Finished calibrating!" << std::endl;

            cout << R << std::endl;
            cout << T << std::endl;

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
            // first, let's check out the timestamps.
            // for (size_t i = 0; i < depth_images.size(); ++i)
            // {
            //     cout << "Color image timestamp: " << i << " " << color_images[i].get_device_timestamp().count() <<
            //     "\n"; cout << "Depth image timestamp: " << i << " " << depth_images[i].get_device_timestamp().count()
            //     << "\n";
            // }

            // let's greenscreen out things that are far away.
            // first: let's get the depth image into the color camera space
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

            cv::Mat output_image(master_color_image.get_height_pixels(),
                                 master_color_image.get_width_pixels(),
                                 CV_8UC3,
                                 cv::Scalar(0, 255, 0));

            // next, let's get some OpenCV images
            cv::Mat master_opencv_color_image = k4a_color_to_opencv(master_color_image);

            // GOTCHA: if you use std::endl to force flushes, you will likely drop frames.
            // now mask it
            const uint16_t THRESHOLD = 1000; // TODO
            cv::Mat mask;
            cv::bitwise_and(opencv_master_depth_in_master_color < THRESHOLD,
                            opencv_master_depth_in_master_color != 0,
                            mask);
            master_opencv_color_image.copyTo(output_image, mask);
            // please?
            // cv::imshow("Test", output_image);
            // cv::waitKey(1);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
