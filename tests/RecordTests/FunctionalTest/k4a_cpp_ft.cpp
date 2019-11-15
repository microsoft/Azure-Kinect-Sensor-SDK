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

TEST_F(k4a_cpp_ft, record)
{
    device kinect = device::open(0);
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.synchronized_images_only = true;
    kinect.start_cameras(&config);
    kinect.start_imu();

    record recorder = record::create("./k4a_cpp_ft.mkv", &kinect, &config);
    {
        record recorder2 = record::create("./k4a_cpp_ft_2.mkv", &kinect, &config);
        ASSERT_TRUE(recorder != recorder2);
        ASSERT_FALSE(recorder == recorder2);

        record recorder4 = std::move(recorder2); // deep copy
        ASSERT_TRUE(recorder4 != nullptr);
        ASSERT_FALSE(recorder4 == nullptr);

        recorder2.close();
        recorder4.close();
    }

    recorder.add_tag("K4A_CPP_FT_ADD_TAG", "K4A_CPP_FT_ADD_TAG");
    recorder.add_imu_track();

    std::string k4a_cpp_ft_attachment = "K4A_CPP_FT_ADD_ATTACHMENT";
    recorder.add_attachment(&k4a_cpp_ft_attachment,
                            (uint8_t *)k4a_cpp_ft_attachment.data(),
                            k4a_cpp_ft_attachment.size());

    k4a_record_video_settings_t vid_settings = { 1920, 1080, 30 };
    std::string k4a_cpp_ft_custom_vid_track = "K4A_CPP_FT_CUSTOM_VID_TRACK";
    recorder.add_custom_video_track(k4a_cpp_ft_custom_vid_track.data(), "V_MPEG1", nullptr, 0, &vid_settings);

    k4a_record_subtitle_settings_t st_track = { false };
    std::string k4a_cpp_ft_custom_subtitle_track = "CUSTOM_K4A_SUBTITLE_TRACE";
    recorder.add_custom_subtitle_track(k4a_cpp_ft_custom_subtitle_track.data(), "V_MPEG1", nullptr, 0, &st_track);

    recorder.write_header();

    for (int x = 0; x < 100; x++)
    {
        capture capture;
        k4a_imu_sample_t imu;
        if (kinect.get_capture(&capture, std::chrono::milliseconds(1000)))
        {
            recorder.write_capture(capture);
        }
        while (kinect.get_imu_sample(&imu, std::chrono::milliseconds(0)))
        {
            recorder.write_imu_sample(&imu);
        }
        image color = capture.get_color_image();
        recorder.write_custom_track_data(k4a_cpp_ft_custom_vid_track.data(),
                                         color.get_device_timestamp(),
                                         color.get_buffer(),
                                         color.get_size());
        color.reset();

        image depth = capture.get_depth_image();
        recorder.write_custom_track_data(k4a_cpp_ft_custom_subtitle_track.data(),
                                         depth.get_device_timestamp(),
                                         depth.get_buffer(),
                                         depth.get_size());
        capture.reset();
    }
    recorder.flush();
    kinect.stop_cameras();
    kinect.stop_imu();
}

TEST_F(k4a_cpp_ft, playback)
{
    playback pb = playback::open("./k4a_cpp_ft.mkv");
    playback pb_empty;
    ASSERT_TRUE(pb);             // bool operation
    ASSERT_TRUE(pb != nullptr);  //!= nullptr
    ASSERT_FALSE(pb == nullptr); // == nullptr
    ASSERT_TRUE(pb != pb_empty);
    ASSERT_FALSE(pb == pb_empty);

    // Deep copy
    playback pback = std::move(pb);
    ASSERT_TRUE(pback != nullptr);
    ASSERT_TRUE(pb == nullptr);

    std::vector<uint8_t> raw_cal = pback.get_raw_calibration();
    std::cout << "calibration is : ";
    for (std::vector<uint8_t>::const_iterator i = raw_cal.begin(); i != raw_cal.end(); ++i)
    {
        std::cout << *i;
    }
    std::cout << "\n\n";

    k4a_record_configuration_t config = pback.get_record_configuration();

    calibration cal = pback.get_calibration();
    {
        device kinect = device::open(0);
        calibration device_cal = kinect.get_calibration(config.depth_mode, config.color_resolution);
        ASSERT_TRUE(cal.color_resolution == device_cal.color_resolution);
        ASSERT_TRUE(cal.depth_mode == device_cal.depth_mode);
    }

    pback.set_color_conversion(K4A_IMAGE_FORMAT_COLOR_NV12);
    pback.set_color_conversion(K4A_IMAGE_FORMAT_COLOR_BGRA32);

    std::chrono::microseconds length = pback.get_recording_length();

    pback.seek_timestamp(std::chrono::microseconds(0), K4A_PLAYBACK_SEEK_BEGIN);
    pback.seek_timestamp(std::chrono::microseconds(0), K4A_PLAYBACK_SEEK_END);
    pback.seek_timestamp(length / 2, K4A_PLAYBACK_SEEK_DEVICE_TIME);

    int capture_count_forward = 0;
    int imu_count_forward = 0;
    int capture_count_backward = 0;
    int imu_count_backward = 0;
    int vid_block_count_forwards = 0;
    int subtitle_block_count_forwards = 0;
    int vid_block_count_backwards = 0;
    int subtitle_block_count_backwards = 0;
    std::string k4a_cpp_ft_custom_vid_track = "K4A_CPP_FT_CUSTOM_VID_TRACK";
    std::string k4a_cpp_ft_custom_subtitle_track = "CUSTOM_K4A_SUBTITLE_TRACE";

    // walk the file forward
    {
        pback.seek_timestamp(std::chrono::microseconds(0), K4A_PLAYBACK_SEEK_BEGIN);

        capture cap;
        data_block block;
        k4a_imu_sample_t imu;

        while (pback.get_next_capture(&cap))
        {
            capture_count_forward++;
        }

        while (pback.get_next_imu_sample(&imu))
        {
            imu_count_forward++;
        }

        while (pback.get_next_data_block(&k4a_cpp_ft_custom_vid_track, &block))
        {
            vid_block_count_forwards++;
        }

        while (pback.get_next_data_block(&k4a_cpp_ft_custom_subtitle_track, &block))
        {
            subtitle_block_count_forwards++;
        }

        ASSERT_GT(capture_count_forward, 0);
        ASSERT_GT(imu_count_forward, 0);
        ASSERT_GT(imu_count_forward, capture_count_forward);
        ASSERT_EQ(capture_count_forward, vid_block_count_forwards);
        ASSERT_EQ(capture_count_forward, subtitle_block_count_forwards);
    }

    // walk the file backwards
    {
        capture cap;
        k4a_imu_sample_t imu;
        data_block block;

        while (pback.get_previous_capture(&cap))
        {
            capture_count_backward++;
        }

        while (pback.get_previous_imu_sample(&imu))
        {
            imu_count_backward++;
        }

        while (pback.get_previous_data_block(&k4a_cpp_ft_custom_vid_track, &block))
        {
            vid_block_count_backwards++;
        }

        while (pback.get_previous_data_block(&k4a_cpp_ft_custom_subtitle_track, &block))
        {
            subtitle_block_count_backwards++;
        }

        ASSERT_GT(capture_count_backward, 0);
        ASSERT_GT(imu_count_backward, 0);
        ASSERT_GT(imu_count_backward, capture_count_backward);
        ASSERT_EQ(capture_count_backward, vid_block_count_backwards);
        ASSERT_EQ(capture_count_backward, subtitle_block_count_backwards);
    }

    ASSERT_EQ(capture_count_forward, capture_count_backward);
    ASSERT_EQ(imu_count_forward, imu_count_backward);

    // walk the file forward after seek to end
    {
        pback.seek_timestamp(std::chrono::microseconds(0), K4A_PLAYBACK_SEEK_END);

        capture cap;
        int capture_count = 0;
        int imu_count = 0;
        int vid_block_count = 0;
        int subtitle_block_count = 0;
        data_block block;

        while (pback.get_next_capture(&cap))
        {
            capture_count++;
        }

        k4a_imu_sample_t imu;
        while (pback.get_next_imu_sample(&imu))
        {
            imu_count++;
        }

        while (pback.get_next_data_block(&k4a_cpp_ft_custom_vid_track, &block))
        {
            vid_block_count++;
            ASSERT_NE(block.get_device_timestamp_usec().count(), 0);
            ASSERT_NE(block.get_buffer_size(), 0);
            ASSERT_NE(block.get_buffer(), nullptr);

            {
                // Test data_block
                data_block copy_same = block;
                data_block copy_null = NULL;
                ASSERT_TRUE(block == copy_same);
                ASSERT_FALSE(block != copy_same);
                ASSERT_FALSE(block == copy_null);
                ASSERT_TRUE(block != copy_null);
            }
        }

        while (pback.get_next_data_block(&k4a_cpp_ft_custom_subtitle_track, &block))
        {
            subtitle_block_count++;
        }

        ASSERT_EQ(capture_count, 0);
        ASSERT_EQ(imu_count, 0);
        ASSERT_EQ(capture_count, vid_block_count);
        ASSERT_EQ(capture_count, subtitle_block_count);
    }

    std::string k4a_cpp_ft_attachment = "K4A_CPP_FT_ADD_ATTACHMENT";
    std::string bad_attachment_name = "BAD_ATTACHMENT_NAME";

    std::vector<uint8_t> data;
    ASSERT_FALSE(pback.get_attachment(&bad_attachment_name, &data));
    ASSERT_TRUE(pback.get_attachment(&k4a_cpp_ft_attachment, &data));
    ASSERT_EQ(data.size(), k4a_cpp_ft_attachment.size());
    ASSERT_TRUE(memcmp(&data[0], k4a_cpp_ft_attachment.data(), data.size()) == 0);
}
