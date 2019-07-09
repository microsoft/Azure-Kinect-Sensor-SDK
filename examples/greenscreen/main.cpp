#include <algorithm>
#include <iostream>
#include <vector>

#include <k4a/k4a.hpp>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>

int main()
{
    // Require at least 2 cameras
    const size_t num_devices = k4a::device::get_installed_count();
    std::cout << num_devices << std::endl;
    if (num_devices < 2)
    {
        throw "At least 2 cameras are required!";
    }
    // We will use devices[0] as the master device
    std::vector<k4a::device> devices;
    devices.emplace_back(k4a::device::open(0));
    devices.emplace_back(k4a::device::open(1));
    // Configure both of the cameras with the same framerate, resolution, exposure, and firmware
    // NOTE: Both cameras must have the same configuration (TODO) what exactly needs to
    k4a_device_configuration_t camera_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    camera_config.color_format = K4A_IMAGE_FORMAT_COLOR_YUY2;
    camera_config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    camera_config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    camera_config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    camera_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
    camera_config.subordinate_delay_off_master_usec = 0; // TODO nope this should be the min, sort through all the
                                                         // syncing
    camera_config.synchronized_images_only = true;
    std::vector<k4a_device_configuration_t> configurations(num_devices, camera_config);
    // special case: the first config needs to be set to be the master
    configurations.front().wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER; // TODO delay for the rest

    std::vector<k4a::calibration> calibrations;
    // fill the calibrations vector
    for (const k4a::device &d : devices)
    {
        calibrations.emplace_back(d.get_calibration(camera_config.depth_mode, camera_config.color_resolution));
    }

    // make sure that everything is plugged in
    if (!devices[0].is_sync_out_connected())
    {
        throw "Out sync needs to be connected to master camera!";
    }
    // make sure that all other devices are ready to receive sync commands from the master camera
    for (auto i = devices.cbegin() + 1; i != devices.cend(); ++i)
    {
        if (!i->is_sync_in_connected())
        {
            throw "Out sync needs to be connected to master camera!";
        }
    }

    // now it's time to start the cameras. All subordinate cameras must be started before the master
    for (size_t i = 1; i < devices.size(); ++i)
    {
        devices[i].start_cameras(&configurations[i]);
    }
    devices[0].start_cameras(&configurations[0]);

    // the cameras have been started. Now let's get the images we need.
    std::vector<k4a::capture> device_captures;
    device_captures.reserve(devices.size());
    for (size_t i = 0; i < devices.size(); ++i)
    {
        if (!devices[i].get_capture(&device_captures[i], std::chrono::milliseconds{ K4A_WAIT_INFINITE }))
        {
            throw "Getting a capture failed!";
        }
    }
    // TODO why does this not compile
    for (k4a::capture &c : device_captures)
    {
        assert(c.get_color_image().get_device_timestamp() ==
               device_captures[0].get_color_image().get_device_timestamp());
        std::cout << c.get_color_image().get_buffer() << std::endl;
    }
}
