#include <algorithm>
#include <iostream>
#include <vector>

#include <k4a/k4a.hpp>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>

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
        std::vector<k4a::device> devices;
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
        std::vector<k4a_device_configuration_t> configurations(num_devices, camera_config);
        // special case: the first config needs to be set to be the master
        configurations.front().wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
        configurations.front().subordinate_delay_off_master_usec = 0; // TODO delay for the rest
        configurations.front().color_resolution = K4A_COLOR_RESOLUTION_1080P;
        configurations.front().synchronized_images_only = true;

        std::vector<k4a::calibration> calibrations;
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
            std::vector<k4a::capture> device_captures(devices.size());
            // first, go through and get a capture from each
            for (size_t i = 0; i < devices.size(); ++i)
            {
                if (!devices[i].get_capture(&device_captures[i], std::chrono::milliseconds{ K4A_WAIT_INFINITE }))
                {
                    throw std::runtime_error("Getting a capture failed!");
                }
            }

            std::cout << "Here!" << std::endl;
            // make sure all of the captures are valid
            if (!std::all_of(device_captures.cbegin(), device_captures.cbegin(), [](const k4a::capture &c) {
                    return c;
                }))
            {
                std::cout << "Not all device captures were valid!" << std::endl;
                continue;
            }

            // TODO the caveat that ownership DOES transfer is NOT obvious ESPECIALLY given that get_color_image, etc,
            // is const get color images
            std::vector<k4a::image> color_images;
            for (const k4a::capture &cap : device_captures)
            {
                color_images.emplace_back(cap.get_color_image());
            }
            if (!std::all_of(color_images.cbegin(), color_images.cend(), [](const k4a::image &i) { return i; }))
            {
                std::cout << "One or more invalid color images!" << std::endl;
                continue;
            }

            // get depth images
            std::vector<k4a::image> depth_images;
            for (const k4a::capture &cap : device_captures)
            {
                depth_images.emplace_back(cap.get_depth_image());
            }
            if (!std::all_of(depth_images.cbegin(), depth_images.cend(), [](const k4a::image &i) { return i; }))
            {
                std::cout << "One or more invalid depth images!" << std::endl;
                continue;
            }
            std::cout << "Success???" << std::endl;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
