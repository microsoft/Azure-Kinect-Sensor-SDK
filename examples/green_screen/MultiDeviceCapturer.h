// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include <chrono>
#include <vector>
#include <k4a/k4a.hpp>

// This is the maximum difference between when we expected an image's timestamp to be and when it actually occurred.
constexpr std::chrono::microseconds MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP(50);

struct MultiDeviceCapturer
{
    // Set up all the devices. Note that the index order isn't necessarily preserved, because we might swap with master
    MultiDeviceCapturer(const vector<int> &device_indices, int32_t color_exposure_usec, int32_t powerline_freq)
    {
        bool master_found = false;
        for (int i : device_indices)
        {
            devices.emplace_back(k4a::device::open(i)); // construct a device using this index
            // If you want to synchronize cameras, you need to manually set both their exposures
            devices.back().set_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                             K4A_COLOR_CONTROL_MODE_MANUAL,
                                             color_exposure_usec);
            // This setting compensates for the flicker of lights due to the frequency of AC power in your region. If
            // you are in an area with 50 Hz power, this may need to be updated (check the docs for
            // k4a_color_control_command_t)
            devices.back().set_color_control(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY,
                                             K4A_COLOR_CONTROL_MODE_MANUAL,
                                             powerline_freq);
            // We treat the first device found with a sync out cable attached as the master. If it's not supposed to be,
            // unplug the cable from it.
            if (devices.back().is_sync_out_connected() && !master_found)
            {
                // If this isn't already the first and only device, make it the first
                if (devices.size() > 1)
                {
                    std::swap(devices.front(), devices.back());
                }
                master_found = true;
            }
            else if (!devices.back().is_sync_in_connected())
            {
                throw std::runtime_error("Non-master camera doesn't have the sync in port connected!");
            }
        }
    }

    // configs[0] should be the master, the rest subordinate
    void start_devices(const std::vector<k4a_device_configuration_t> &configs)
    {
        if (devices.size() != configs.size())
        {
            throw std::runtime_error("Size of configurations not the same as size of devices!");
        }
        // Start by starting all of the subordinate devices. They must be started before the master!
        for (size_t i = 1; i < devices.size(); ++i)
        {
            devices[i].start_cameras(&configs[i]);
        }
        // Lastly, start the master device
        devices[0].start_cameras(&configs[0]);
    }

    // Blocks until we have synchronized captures stored in the output
    std::vector<k4a::capture> get_synchronized_captures(const vector<k4a_device_configuration_t> configs,
                                                        bool compare_sub_depth_instead_of_color = false)
    {
        // Dealing with the synchronized cameras is complex. The Azure Kinect DK:
        //      (a) does not guarantee exactly equal timestamps between depth and color or between cameras (delays can
        //      be configured but timestamps will only be approximately the same)
        //      (b) does not guarantee that, if the two most recent images were synchronized, that calling get_capture
        //      just once on each camera will still be synchronized.
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
        std::vector<k4a::capture> captures(devices.size());
        for (size_t i = 0; i < devices.size(); ++i)
        {
            devices[i].get_capture(&captures[i], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
        }

        bool have_synced_images = false;
        while (!have_synced_images)
        {
            k4a::image master_color_image = captures[0].get_color_image();
            std::chrono::microseconds master_color_image_time = master_color_image.get_device_timestamp();

            for (size_t i = 1; i < devices.size(); ++i)
            {
                k4a::image sub_image;
                if (compare_sub_depth_instead_of_color)
                {
                    sub_image = captures[i].get_depth_image();
                }
                else
                {
                    sub_image = captures[i].get_color_image();
                }

                if (master_color_image && sub_image)
                {
                    std::chrono::microseconds sub_image_time = sub_image.get_device_timestamp();
                    // The subordinate's color image timestamp, ideally, is the master's color image timestamp plus the
                    // delay we configured between the master device color camera and subordinate device color camera
                    std::chrono::microseconds expected_sub_image_time =
                        master_color_image_time +
                        std::chrono::microseconds{ configs[i].subordinate_delay_off_master_usec } +
                        std::chrono::microseconds{ configs[i].depth_delay_off_color_usec };
                    std::chrono::microseconds sub_image_time_error = sub_image_time - expected_sub_image_time;
                    // The time error's absolute value must be within the permissible range. So, for example, if
                    // MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP is 2, offsets of -2, -1, 0, 1, and -2 are
                    // permitted
                    if (sub_image_time_error < -MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP)
                    {
                        // Example, where MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP is 1
                        // time                    t=1  t=2  t=3
                        // actual timestamp        x    .    .
                        // expected timestamp      .    .    x
                        // error: 1 - 3 = -2, which is less than the worst-case-allowable offset of -1
                        // the subordinate camera image timestamp was earlier than it is allowed to be. This means the
                        // subordinate is lagging and we need to update the subordinate to get the subordinate caught up
                        std::cout << "Subordinate lagging...\n";
                        devices[i].get_capture(&captures[i], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
                        break;
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
                        std::cout << "Master lagging...\n";
                        devices[0].get_capture(&captures[0], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
                        break;
                    }
                    else
                    {
                        // These captures are sufficiently synchronized. If we've gotten to the end, then all are
                        // synchronized.
                        if (i == devices.size() - 1)
                        {
                            have_synced_images = true; // now we'll finish the for loop and then exit the while loop
                        }
                    }
                }
                else if (!master_color_image)
                {
                    std::cout << "Master image was bad!\n";
                    devices[0].get_capture(&captures[0], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
                    break;
                }
                else if (!sub_image)
                {
                    std::cout << "Subordinate image was bad!" << endl;
                    devices[i].get_capture(&captures[i], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
                    break;
                }
            }
        }
        // if we've made it to here, it means that we have synchronized captures.
        return captures;
    }

    // Once the constuctor finishes, devices[0] will always be the master
    std::vector<k4a::device> devices;
};
