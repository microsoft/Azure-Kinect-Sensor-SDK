#include <algorithm>
#include <iostream>
#include <vector>

#include <k4a/k4a.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>

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
        camera_config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
        camera_config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
        camera_config.camera_fps = K4A_FRAMES_PER_SECOND_30;
        camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE; // TODO SUBORDINATE
        camera_config.subordinate_delay_off_master_usec = 0; // TODO nope this should be the min, sort through all
                                                             // the syncing
        camera_config.synchronized_images_only = false;
        vector<k4a_device_configuration_t> configurations(num_devices, camera_config);
        // special case: the first config needs to be set to be the master
        configurations.front().wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
        configurations.front().subordinate_delay_off_master_usec = 0; // TODO delay for the rest
        configurations.front().color_resolution = K4A_COLOR_RESOLUTION_1080P;
        configurations.front().synchronized_images_only = true;

        vector<k4a::calibration> calibrations;
        // fill the calibrations vector
        for (const k4a::device &d : devices)
        {
            calibrations.emplace_back(d.get_calibration(camera_config.depth_mode, camera_config.color_resolution));
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

        // the cameras have been started. Now let's get the images we need.

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

            vector<k4a::image> color_images;
            for (const k4a::capture &cap : device_captures)
            {
                color_images.emplace_back(cap.get_color_image());
            }
            if (!std::all_of(color_images.cbegin(), color_images.cend(), [](const k4a::image &i) { return i; }))
            {
                cout << "One or more invalid color images!\n"; // << std::endl;
                continue;
            }

            // get depth images
            vector<k4a::image> depth_images;
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
            for (size_t i = 0; i < depth_images.size(); ++i)
            {
                cout << "Color image timestamp: " << i << " " << color_images[i].get_device_timestamp().count() << "\n";
                cout << "Depth image timestamp: " << i << " " << depth_images[i].get_device_timestamp().count() << "\n";
            }

            // let's greenscreen out things that are far away.
            // first: let's get the depth image into the color camera space
            // create a copy with the same parameters
            cout << color_images[0].get_width_pixels() << std::endl;
            cout << color_images[0].get_height_pixels() << std::endl;
            cout << color_images[0].get_stride_bytes() << std::endl;
            k4a::image k4a_master_depth_in_master_color = k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16,
                                                                             color_images[0].get_width_pixels(),
                                                                             color_images[0].get_height_pixels(),
                                                                             color_images[0].get_stride_bytes());
            // now fill it with the shifted version
            // to do so, we need the transformation from
            cout << "Test" << std::endl;
            k4a::transformation master_depth_to_master_color(calibrations[0]);
            cout << "Test" << std::endl;
            master_depth_to_master_color.depth_image_to_color_camera(depth_images[0],
                                                                     &k4a_master_depth_in_master_color);
            cout << "Test2" << std::endl;

            // now, create an OpenCV version of the depth matrix for easy usage
            cv::Mat opencv_master_depth_in_master_color = k4a_depth_to_opencv(depth_images[0]);

            for (int w = 0; w < opencv_master_depth_in_master_color.cols; ++w)
            {
                for (int h = 0; h < opencv_master_depth_in_master_color.rows; ++h)
                {
                    cout << opencv_master_depth_in_master_color.at<uint16_t>(w, h);
                }
            }

            // next, let's get some OpenCV images
            vector<cv::Mat> opencv_color_images;
            opencv_color_images.reserve(color_images.size());
            for (const k4a::image &im : color_images)
            {
                opencv_color_images.emplace_back(k4a_color_to_opencv(im));
            }
            // please?
            cv::imshow("Test", opencv_color_images[0]);
            cv::waitKey(0);

            // GOTCHA: if you use std::endl to force flushes, you will likely drop frames.
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
