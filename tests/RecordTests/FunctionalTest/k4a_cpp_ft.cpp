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

#if 1
TEST_F(k4a_cpp_ft, k4a)
{
    device kinect = device::open(0);

    ASSERT_THROW(device kinect2 = device::open(0), error);
    kinect.close();
    kinect = kinect.open(0);

    {
        ASSERT_LE((uint32_t)1, device::get_installed_count());
    }

    {
        k4a_hardware_version_t version = kinect.get_version();
        ASSERT_GE(version.rgb.major, (uint32_t)1);
        ASSERT_GE(version.rgb.minor, (uint32_t)6);
        ASSERT_GE(version.depth.major, (uint32_t)1);
        ASSERT_GE(version.depth.minor, (uint32_t)6);
        ASSERT_GE(version.audio.major, (uint32_t)1);
        ASSERT_GE(version.audio.minor, (uint32_t)6);
        ASSERT_EQ(version.firmware_build, K4A_FIRMWARE_BUILD_RELEASE);
        ASSERT_EQ(version.firmware_signature, K4A_FIRMWARE_SIGNATURE_MSFT);
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
        calibration cal = kinect.get_calibration(K4A_DEPTH_MODE_NFOV_2X2BINNED, K4A_COLOR_RESOLUTION_1440P);
        calibration::get_from_raw(raw_cal.data(),
                                  raw_cal.size(),
                                  K4A_DEPTH_MODE_NFOV_2X2BINNED,
                                  K4A_COLOR_RESOLUTION_1080P);
    }

    {
        k4a_color_control_mode_t mode;
        int32_t value;
        kinect.set_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, K4A_COLOR_CONTROL_MODE_AUTO, 0);
        kinect.get_color_control(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &mode, &value);
        ASSERT_EQ(K4A_COLOR_CONTROL_MODE_AUTO, mode);
    }

    {
        std::string sernum = kinect.get_serialnum();
    }

    {
        k4a_imu_sample_t sample = { 0 };
        capture cap1, cap2;
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
        config.depth_mode = K4A_DEPTH_MODE_PASSIVE_IR;
        config.synchronized_images_only = true;
        kinect.start_cameras(&config);
        kinect.start_imu();
        ASSERT_TRUE(kinect.get_capture(&cap1));
        ASSERT_TRUE(kinect.get_capture(&cap2));
        ASSERT_TRUE(kinect.get_imu_sample(&sample));
        ASSERT_TRUE(cap1 != cap2);
        kinect.stop_cameras();
        kinect.stop_imu();

        ASSERT_NE(sample.acc_timestamp_usec, 0);
        ASSERT_NE(sample.gyro_timestamp_usec, 0);

        ASSERT_LT(0, cap1.get_temperature_c());
        ASSERT_LT(0, cap2.get_temperature_c());
        cap1.set_temperature_c(0.0f);
        cap2.set_temperature_c(0.0f);

        {
            capture shallow_copy;
            capture deep_copy;
            shallow_copy = cap1;               // test = operator
            ASSERT_TRUE(shallow_copy == cap1); // test == operator

            deep_copy = std::move(cap1);       // test = (&&) operator
            ASSERT_TRUE(cap1 == nullptr);      // test == nullptr_t operator
            ASSERT_TRUE(cap1 == cap1);         // test == operator
            ASSERT_TRUE(deep_copy != nullptr); // test != nullptr_t operator
            ASSERT_TRUE(deep_copy != cap2);    // test != operator

            // restore cap1
            cap1 = std::move(deep_copy); // test = (&&) operator
        }

        image color, ir, depth;
        color = cap1.get_color_image();
        ir = cap1.get_ir_image();
        depth = cap1.get_depth_image();
        ASSERT_TRUE(depth == nullptr);  // test == operator
        ASSERT_FALSE(depth != nullptr); // test != operator

        {
            image shallow_copy;
            image deep_copy;
            shallow_copy = color;               // test = operator
            ASSERT_TRUE(shallow_copy == color); // test == operator

            deep_copy = std::move(color);      // test = (&&) operator
            ASSERT_TRUE(color == nullptr);     // test == nullptr_t operator
            ASSERT_TRUE(color == color);       // test == operator
            ASSERT_TRUE(deep_copy != nullptr); // test != nullptr_t operator
            ASSERT_TRUE(deep_copy != ir);      // test != operator

            // restore color
            color = std::move(deep_copy); // test = (&&) operator
        }

        {
            // test .reset
            image im = image::create(K4A_IMAGE_FORMAT_COLOR_NV12, 1024, 768, 1024 * 3);
            im.reset();
            ASSERT_TRUE(im == nullptr);
            im.reset(); // should be safe
        }

        {
            // test .set_XXX_image && .reset
            image im = image::create(K4A_IMAGE_FORMAT_COLOR_NV12, 1024, 768, 1024 * 3);
            capture temp_cap = capture::create();
            temp_cap.set_color_image(im);
            temp_cap.set_ir_image(im);
            ASSERT_TRUE(temp_cap.get_color_image() == im);
            ASSERT_TRUE(temp_cap.get_ir_image() == im);
            ASSERT_TRUE(temp_cap.get_depth_image() == nullptr);

            temp_cap.set_color_image(nullptr);
            temp_cap.set_ir_image(nullptr);
            temp_cap.set_depth_image(im);
            ASSERT_TRUE(temp_cap.get_color_image() == nullptr);
            ASSERT_TRUE(temp_cap.get_ir_image() == nullptr);
            ASSERT_TRUE(temp_cap.get_depth_image() == im);

            temp_cap.reset();
            ASSERT_TRUE(temp_cap == nullptr);
        }

        uint8_t *non_const_null = nullptr;
        ASSERT_TRUE(color.get_buffer() != non_const_null);
        ASSERT_TRUE(color.get_buffer() != nullptr);

        ASSERT_TRUE(color.get_size() > 0);
        ASSERT_TRUE(color.get_format() == K4A_IMAGE_FORMAT_COLOR_MJPG);
        ASSERT_TRUE(color.get_width_pixels() == 1920);
        ASSERT_TRUE(color.get_height_pixels() == 1080);
        ASSERT_TRUE(color.get_stride_bytes() == 0);
        ASSERT_TRUE(color.get_device_timestamp() != std::chrono::microseconds(0));
        ASSERT_TRUE(color.get_system_timestamp() != std::chrono::microseconds(0));
        ASSERT_TRUE(color.get_exposure() != std::chrono::microseconds(0));
        ASSERT_TRUE(color.get_white_balance() != 0);
        ASSERT_TRUE(color.get_iso_speed() != 0);

        // test that they don't fail
        color.set_timestamp(std::chrono::microseconds(0x1234));
        color.set_exposure_time(std::chrono::microseconds(500));
        color.set_white_balance(500);
        color.set_iso_speed(500);
    }
}
#endif