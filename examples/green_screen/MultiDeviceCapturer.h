// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include <chrono>
#include <vector>
#include <iomanip> // std::setw
#include <k4a/k4a.hpp>

// This is the maximum difference between when we expected an image's timestamp to be and when it actually occurred.
constexpr std::chrono::microseconds MAX_ALLOWABLE_TIME_OFFSET_ERROR_FOR_IMAGE_TIMESTAMP(100);

constexpr int64_t WAIT_FOR_SYNCHRONIZED_CAPTURE_TIMEOUT = 60000;

static void log_lagging_time(const char *lagger, k4a::capture &master, k4a::capture &sub)
{
    std::cout << std::setw(6) << lagger << " lagging: mc:" << std::setw(6)
              << master.get_color_image().get_device_timestamp().count() << "us sc:" << std::setw(6)
              << sub.get_color_image().get_device_timestamp().count() << "us\n";
}

static void log_synced_image_time(k4a::capture &master, k4a::capture &sub)
{
    std::cout << "Sync'd capture: mc:" << std::setw(6) << master.get_color_image().get_device_timestamp().count()
              << "us sc:" << std::setw(6) << sub.get_color_image().get_device_timestamp().count() << "us\n";
}

class MultiDeviceCapturer
{
public:
    // Set up all the devices. Note that the index order isn't necessarily preserved, because we might swap with master
    MultiDeviceCapturer(const vector<uint32_t> &device_indices, int32_t color_exposure_usec, int32_t powerline_freq)
    {
        bool master_found = false;
        if (device_indices.size() == 0)
        {
            cerr << "Capturer must be passed at least one camera!\n ";
            exit(1);
        }
        for (uint32_t i : device_indices)
        {
            k4a::device next_device = k4a::device::open(i); // construct a device using this index
            // If you want to synchronize cameras, you need to manually set both their exposures
            next_device.set_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                          K4A_COLOR_CONTROL_MODE_MANUAL,
                                          color_exposure_usec);
            // This setting compensates for the flicker of lights due to the frequency of AC power in your region. If
            // you are in an area with 50 Hz power, this may need to be updated (check the docs for
            // k4a_color_control_command_t)
            next_device.set_color_control(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY,
                                          K4A_COLOR_CONTROL_MODE_MANUAL,
                                          powerline_freq);
            // We treat the first device found with a sync out cable attached as the master. If it's not supposed to be,
            // unplug the cable from it. Also, if there's only one device, just use it
            if ((next_device.is_sync_out_connected() && !master_found) || device_indices.size() == 1)
            {
                master_device = std::move(next_device);
                master_found = true;
            }
            else if (!next_device.is_sync_in_connected() && !next_device.is_sync_out_connected())
            {
                cerr << "Each device must have sync in or sync out connected!\n ";
                exit(1);
            }
            else if (!next_device.is_sync_in_connected())
            {
                cerr << "Non-master camera found that doesn't have the sync in port connected!\n ";
                exit(1);
            }
            else
            {
                subordinate_devices.emplace_back(std::move(next_device));
            }
        }
        if (!master_found)
        {
            cerr << "No device with sync out connected found!\n ";
            exit(1);
        }
    }

    // configs[0] should be the master, the rest subordinate
    void start_devices(const k4a_device_configuration_t &master_config, const k4a_device_configuration_t &sub_config)
    {
        // Start by starting all of the subordinate devices. They must be started before the master!
        for (k4a::device &d : subordinate_devices)
        {
            d.start_cameras(&sub_config);
        }
        // Lastly, start the master device
        master_device.start_cameras(&master_config);
    }

    // Blocks until we have synchronized captures stored in the output. First is master, rest are subordinates
    std::vector<k4a::capture> get_synchronized_captures(const k4a_device_configuration_t &sub_config,
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
        // The captures are stored in a vector where the first element of the vector is the master capture and
        // subsequent elements are subordinate captures
        std::vector<k4a::capture> captures(subordinate_devices.size() + 1); // add 1 for the master
        size_t current_index = 0;
        master_device.get_capture(&captures[current_index], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
        ++current_index;
        for (k4a::device &d : subordinate_devices)
        {
            d.get_capture(&captures[current_index], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
            ++current_index;
        }

        // If there are no subordinate devices, just return captures which only has the master image
        if (subordinate_devices.empty())
        {
            return captures;
        }

        bool have_synced_images = false;
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        while (!have_synced_images)
        {
            // Timeout if this is taking too long
            int64_t duration_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
            if (duration_ms > WAIT_FOR_SYNCHRONIZED_CAPTURE_TIMEOUT)
            {
                cerr << "ERROR: Timedout waiting for synchronized captures\n";
                exit(1);
            }

            k4a::image master_color_image = captures[0].get_color_image();
            std::chrono::microseconds master_color_image_time = master_color_image.get_device_timestamp();

            for (size_t i = 0; i < subordinate_devices.size(); ++i)
            {
                k4a::image sub_image;
                if (compare_sub_depth_instead_of_color)
                {
                    sub_image = captures[i + 1].get_depth_image(); // offset of 1 because master capture is at front
                }
                else
                {
                    sub_image = captures[i + 1].get_color_image(); // offset of 1 because master capture is at front
                }

                if (master_color_image && sub_image)
                {
                    std::chrono::microseconds sub_image_time = sub_image.get_device_timestamp();
                    // The subordinate's color image timestamp, ideally, is the master's color image timestamp plus the
                    // delay we configured between the master device color camera and subordinate device color camera
                    std::chrono::microseconds expected_sub_image_time =
                        master_color_image_time +
                        std::chrono::microseconds{ sub_config.subordinate_delay_off_master_usec } +
                        std::chrono::microseconds{ sub_config.depth_delay_off_color_usec };
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
                        log_lagging_time("sub", captures[0], captures[i + 1]);
                        subordinate_devices[i].get_capture(&captures[i + 1],
                                                           std::chrono::milliseconds{ K4A_WAIT_INFINITE });
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
                        log_lagging_time("master", captures[0], captures[i + 1]);
                        master_device.get_capture(&captures[0], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
                        break;
                    }
                    else
                    {
                        // These captures are sufficiently synchronized. If we've gotten to the end, then all are
                        // synchronized.
                        if (i == subordinate_devices.size() - 1)
                        {
                            log_synced_image_time(captures[0], captures[i + 1]);
                            have_synced_images = true; // now we'll finish the for loop and then exit the while loop
                        }
                    }
                }
                else if (!master_color_image)
                {
                    std::cout << "Master image was bad!\n";
                    master_device.get_capture(&captures[0], std::chrono::milliseconds{ K4A_WAIT_INFINITE });
                    break;
                }
                else if (!sub_image)
                {
                    std::cout << "Subordinate image was bad!" << endl;
                    subordinate_devices[i].get_capture(&captures[i + 1],
                                                       std::chrono::milliseconds{ K4A_WAIT_INFINITE });
                    break;
                }
            }
        }
        // if we've made it to here, it means that we have synchronized captures.
        return captures;
    }

    const k4a::device &get_master_device() const
    {
        return master_device;
    }

    const k4a::device &get_subordinate_device_by_index(size_t i) const
    {
        // devices[0] is the master. There are only devices.size() - 1 others. So, indices greater or equal are invalid
        if (i >= subordinate_devices.size())
        {
            cerr << "Subordinate index too large!\n ";
            exit(1);
        }
        return subordinate_devices[i];
    }

private:
    // Once the constuctor finishes, devices[0] will always be the master
    k4a::device master_device;
    std::vector<k4a::device> subordinate_devices;
};
