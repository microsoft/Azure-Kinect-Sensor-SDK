// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4arecord/record.hpp>
#include <k4arecord/playback.hpp>
#include <utcommon.h>

#include <gtest/gtest.h>

//#include <azure_c_shared_utility/lock.h>
//#include <azure_c_shared_utility/threadapi.h>
//#include <azure_c_shared_utility/condition.h>

using namespace k4a;

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}

class k4a_cpp_ft : public ::testing::Test
{
public:
    virtual void SetUp() {}

    virtual void TearDown() {}
};

TEST_F(k4a_cpp_ft, record)
{
    device kinect = device::open(0);

    ASSERT_THROW(device kinect2 = device::open(0), error);
    kinect.close();
    kinect = kinect.open(0);

    {
        ASSERT_GE(1, device::get_installed_count());
    }

    {
        k4a_hardware_version_t version = kinect.get_version();
        ASSERT_GE(version.rgb.major, 1);
        ASSERT_GE(version.rgb.minor, 6);
        ASSERT_GE(version.depth.major, 1);
        ASSERT_GE(version.depth.minor, 6);
        ASSERT_GE(version.audio.major, 1);
        ASSERT_GE(version.audio.minor, 6);
        ASSERT_EQ(version.firmware_build, K4A_FIRMWARE_BUILD_RELEASE);
        ASSERT_EQ(version.audio.minor, K4A_FIRMWARE_SIGNATURE_MSFT);
    }

    // should not throw exception
    (void)kinect.is_sync_out_connected();
    (void)kinect.is_sync_in_connected();

    {
        // should not throw exception
        calibration cal = kinect.get_calibration(K4A_DEPTH_MODE_NFOV_2X2BINNED, K4A_COLOR_RESOLUTION_1440P);
    }

    {
        std::vector<uint8_t> raw_cal = kinect.get_raw_calibration();
    }

    {
        k4a_color_control_mode_t mode;
        uint32_t value;
        kinect.set_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, K4A_COLOR_CONTROL_MODE_AUTO, 0);
        kinect.get_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &mode, &value);
        ASSERT_EQ(K4A_COLOR_CONTROL_MODE_AUTO, mode);
        ASSERT_EQ(value, 0);
    }

    {
        std::string sernum = kinect.get_serialnum();
    }

    {
        k4a_imu_sample_t sample = { 0 };
        kinect.start_imu();
        ASSERT_EQ(1, kinect.get_imu_sample(sample));
        kinect.stop_imu();
        ASSERT_NE(sample.acc_timestamp_usec, 0);
        ASSERT_NE(sample.gyro_timestamp_usec, 0);
    }

    {
        capture cap1, cap2;
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
        config.depth_mode = K4A_DEPTH_MODE_PASSIVE_IR;
        config.synchronized_images_only = true;
        kinect.start_cameras(&config);
        ASSERT_EQ(1, kinect.get_capture(&cap1));
        ASSERT_EQ(1, kinect.get_capture(&cap2));
        ASSERT_TRUE(cap1 != cap2)
        kinect.stop_cameras();

        assert_lt(0, kinect.get_temperature_c());
        kinect.set_temperature_c(0.0f);

        image color, ir, depth;
        color = cap1.get_color_image();
        ir = cap1.get_ir_image();
        depth = cap1.get_depth_image();

            capture extracopy = cap1;
            image cp_color, cp_ir, cp_depth;

            ASSERT_TRUE(extracopy == cap1);
            cp_color = extracopy.get_color_image();
            cp_ir = extracopy.get_ir_image();
            cp_depth = extracopy.get_depth_image();
            ASSERT_NE(cp_color, color);
            ASSERT_NE(cp_ir, ir);
            ASSERT_NE(cp_depth, depth);

            // ir should be released;
            cp_ir = cp_depth;

            extracopy = cap2;
            ASSERT_TRUE(extracopy== cap2);
            cp_color = extracopy.get_color_image();
            cp_ir = extracopy.get_ir_image();
            cp_depth = extracopy.get_depth_image();
            ASSERT_NE(cp_color, color);
            ASSERT_NE(cp_ir, ir);
            ASSERT_NE(cp_depth, depth);

            color
    }
}