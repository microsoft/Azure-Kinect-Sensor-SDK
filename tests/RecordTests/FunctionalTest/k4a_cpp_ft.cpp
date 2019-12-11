// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4arecord/record.hpp>
#include <k4arecord/playback.hpp>
#include <utcommon.h>

#include <gtest/gtest.h>

using namespace k4a;

const std::string MKV_FILE_NAME("./k4a_cpp_ft.mkv");
const std::string MKV_FILE_NAME_2ND("./k4a_cpp_ft_2.mkv");

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}

class k4a_cpp_ft : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        // remove old test files, incase old test run crashed
        std::remove(MKV_FILE_NAME.c_str());
        std::remove(MKV_FILE_NAME_2ND.c_str());
    }

    virtual void TearDown() {}
};

static void use_device_in_a_function(const device &kinect)
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

TEST_F(k4a_cpp_ft, k4a)
{
    device kinect = device::open(0);
    ASSERT_TRUE(kinect);
    ASSERT_TRUE(kinect.is_valid());
    kinect.close();
    ASSERT_FALSE(kinect);
    ASSERT_FALSE(kinect.is_valid());

    kinect = device::open(0);
    ASSERT_TRUE(kinect);
    ASSERT_TRUE(kinect.is_valid());

    {
        device kinect2;
        ASSERT_FALSE(kinect2);
    }

    ASSERT_THROW(device kinect2 = device::open(0), error);
    kinect.close();
    kinect = kinect.open(0);

    {
        ASSERT_LE((uint32_t)1, device::get_installed_count());
    }

    {
        // assignment operation deleted, make sure we can still pass
        use_device_in_a_function(kinect);

        // device kinect3 = kinect; // This assignment operator is deleted.
    }

    // should not throw exception
    (void)kinect.is_sync_out_connected();
    (void)kinect.is_sync_in_connected();

    {
        // should not throw exception
        calibration cal = kinect.get_calibration(K4A_DEPTH_MODE_NFOV_2X2BINNED, K4A_COLOR_RESOLUTION_1440P);
        calibration cal2 = cal;
        ASSERT_EQ(cal.color_resolution, cal2.color_resolution);
    }

    {
        std::vector<uint8_t> raw_cal = kinect.get_raw_calibration();
        calibration cal = kinect.get_calibration(K4A_DEPTH_MODE_NFOV_2X2BINNED, K4A_COLOR_RESOLUTION_1440P);
        ASSERT_EQ(cal.color_resolution, K4A_COLOR_RESOLUTION_1440P);

        cal = calibration::get_from_raw(raw_cal.data(),
                                        raw_cal.size(),
                                        K4A_DEPTH_MODE_NFOV_2X2BINNED,
                                        K4A_COLOR_RESOLUTION_1080P);
        ASSERT_EQ(cal.color_resolution, K4A_COLOR_RESOLUTION_1080P);
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
            capture moved_copy;
            shallow_copy = cap1;               // test = operator
            ASSERT_TRUE(shallow_copy == cap1); // test == operator

            moved_copy = std::move(cap1);       // test = (&&) operator
            ASSERT_TRUE(cap1 == nullptr);       // test == nullptr_t operator
            ASSERT_TRUE(cap1 == cap1);          // test == operator
            ASSERT_TRUE(moved_copy != nullptr); // test != nullptr_t operator
            ASSERT_TRUE(moved_copy != cap2);    // test != operator

            ASSERT_EQ(0, moved_copy.get_temperature_c()); // use moved copy
            moved_copy.set_temperature_c(10.0f);          // use moved copy

            // restore cap1
            cap1 = std::move(moved_copy); // test = (&&) operator
        }

        image color, ir, depth;
        color = cap1.get_color_image();
        ir = cap1.get_ir_image();
        depth = cap1.get_depth_image();
        ASSERT_TRUE(depth == nullptr);  // test == operator
        ASSERT_FALSE(depth != nullptr); // test != operator

        {
            image shallow_copy;
            image moved_copy;
            shallow_copy = color;               // test = operator
            ASSERT_TRUE(shallow_copy == color); // test == operator

            moved_copy = std::move(color);      // test = (&&) operator
            ASSERT_TRUE(color == nullptr);      // test == nullptr_t operator
            ASSERT_TRUE(color == color);        // test == operator
            ASSERT_TRUE(moved_copy != nullptr); // test != nullptr_t operator
            ASSERT_TRUE(moved_copy != ir);      // test != operator

            // restore color
            color = std::move(moved_copy); // test = (&&) operator
        }

        {
            // Capture class bool operation, is_valid(), and reset()
            ASSERT_TRUE(cap1);
            ASSERT_TRUE(cap1.is_valid());
            cap1.reset();
            ASSERT_FALSE(cap1);
            ASSERT_FALSE(cap1.is_valid());
            cap1.reset(); // should not crash
            ASSERT_FALSE(cap1);
            ASSERT_FALSE(cap1.is_valid());
        }

        {
            // test reset(), bool operator, is_valid()
            image im = image::create(K4A_IMAGE_FORMAT_COLOR_NV12, 1024, 768, 1024 * 3);
            ASSERT_TRUE(im);
            ASSERT_TRUE(im.is_valid());
            im.reset();
            ASSERT_TRUE(im == nullptr);
            ASSERT_FALSE(im);
            ASSERT_FALSE(im.is_valid());
            im.reset(); // should not crash
            ASSERT_TRUE(im == nullptr);
            ASSERT_FALSE(im);
            ASSERT_FALSE(im.is_valid());
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

    kinect.close();
}

static void test_record(void)
{
    record recorder;
    device kinect = device::open(0);
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.synchronized_images_only = true;
    kinect.start_cameras(&config);
    kinect.start_imu();

    {
        // Test bool operator, close(), is_valid()
        recorder = record::create(MKV_FILE_NAME.c_str(), kinect, config);
        ASSERT_TRUE(recorder);
        ASSERT_TRUE(recorder.is_valid());
        recorder.close();
        ASSERT_FALSE(recorder);
        ASSERT_FALSE(recorder.is_valid());
        recorder.close(); // should not crash
        ASSERT_FALSE(recorder);
        ASSERT_FALSE(recorder.is_valid());
    }

    recorder = record::create(MKV_FILE_NAME.c_str(), kinect, config);
    ASSERT_TRUE(recorder);

    {
        record recorder2 = record::create(MKV_FILE_NAME_2ND.c_str(), kinect, config);
        ASSERT_TRUE(recorder2);

        record recorder_empty;
        ASSERT_FALSE(recorder_empty);

        record recorder_moved = std::move(recorder2);

        recorder_empty.close();
        recorder_moved.close();
        std::remove(MKV_FILE_NAME_2ND.c_str());
    }

    recorder.add_tag("K4A_CPP_FT_ADD_TAG", "K4A_CPP_FT_ADD_TAG");
    recorder.add_imu_track();

    std::string k4a_cpp_ft_attachment = "K4A_CPP_FT_ADD_ATTACHMENT";
    recorder.add_attachment(k4a_cpp_ft_attachment.c_str(),
                            (const uint8_t *)k4a_cpp_ft_attachment.data(),
                            k4a_cpp_ft_attachment.size());

    k4a_record_video_settings_t vid_settings = { 1920, 1080, 30 };
    std::string k4a_cpp_ft_custom_vid_track = "K4A_CPP_FT_CUSTOM_VID_TRACK";
    recorder.add_custom_video_track(k4a_cpp_ft_custom_vid_track.c_str(), "V_MPEG1", nullptr, 0, &vid_settings);

    k4a_record_subtitle_settings_t st_track = { false };
    std::string k4a_cpp_ft_custom_subtitle_track = "CUSTOM_K4A_SUBTITLE_TRACE";
    recorder.add_custom_subtitle_track(k4a_cpp_ft_custom_subtitle_track.c_str(), "V_MPEG1", nullptr, 0, &st_track);

    recorder.write_header();

    for (int x = 0; x < 100; x++)
    {
        capture capture;
        k4a_imu_sample_t imu;
        if (kinect.get_capture(&capture, std::chrono::milliseconds(1000)))
        {
            recorder.write_capture(capture);
        }

        auto start = std::chrono::high_resolution_clock::now();
        while (kinect.get_imu_sample(&imu, std::chrono::milliseconds(0)))
        {
            recorder.write_imu_sample(imu);
            if (std::chrono::high_resolution_clock::now() - start < std::chrono::milliseconds(100))
            {
                break;
            }
        }
        image color = capture.get_color_image();
        recorder.write_custom_track_data(k4a_cpp_ft_custom_vid_track.c_str(),
                                         color.get_device_timestamp(),
                                         color.get_buffer(),
                                         color.get_size());
        color.reset();

        image depth = capture.get_depth_image();
        recorder.write_custom_track_data(k4a_cpp_ft_custom_subtitle_track.c_str(),
                                         depth.get_device_timestamp(),
                                         depth.get_buffer(),
                                         depth.get_size());
        capture.reset();
    }
    recorder.flush();
    kinect.stop_cameras();
    kinect.stop_imu();
}

static void test_playback(void)
{
    playback pb = playback::open(MKV_FILE_NAME.c_str());
    ASSERT_TRUE(pb); // bool operation
    ASSERT_TRUE(pb.is_valid());

    {
        playback pb_missing_file;
        ASSERT_THROW(pb_missing_file = playback::open("./This_file_is_not_here.mkv"), error);

        playback pb_empty;
        ASSERT_FALSE(pb_empty); // bool operation
        ASSERT_FALSE(pb_empty.is_valid());
        ASSERT_FALSE(pb_missing_file); // bool operation
        ASSERT_FALSE(pb_missing_file.is_valid());

        pb.close();
        ASSERT_FALSE(pb); // bool operation
        ASSERT_FALSE(pb.is_valid());
        pb = playback::open(MKV_FILE_NAME.c_str());
    }

    {
        // playback pb_empty = pb; // deleted operation
    }

    playback pback;
    ASSERT_TRUE(pb);     // bool operation
    ASSERT_FALSE(pback); // bool operation
    pback = std::move(pb);
    ASSERT_FALSE(pb);   // bool operation
    ASSERT_TRUE(pback); // bool operation

    std::vector<uint8_t> raw_cal = pback.get_raw_calibration();
    std::cout << "calibration is : ";
    for (const uint8_t &data_char : raw_cal)
    {
        std::cout << data_char;
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
    int vid_block_count_forward = 0;
    int subtitle_block_count_forward = 0;
    int vid_block_count_backward = 0;
    int subtitle_block_count_backward = 0;
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

        while (pback.get_next_data_block(k4a_cpp_ft_custom_vid_track.c_str(), &block))
        {
            vid_block_count_forward++;
        }

        while (pback.get_next_data_block(k4a_cpp_ft_custom_subtitle_track.c_str(), &block))
        {
            subtitle_block_count_forward++;

            ASSERT_TRUE(block);
            ASSERT_TRUE(block.is_valid());
            block.reset();
            ASSERT_FALSE(block);
            ASSERT_FALSE(block.is_valid());
        }

        ASSERT_GT(capture_count_forward, 0);
        ASSERT_GT(imu_count_forward, 0);
        ASSERT_EQ(imu_count_forward, capture_count_forward);
        ASSERT_EQ(capture_count_forward, vid_block_count_forward);
        ASSERT_EQ(capture_count_forward, subtitle_block_count_forward);
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

        while (pback.get_previous_data_block(k4a_cpp_ft_custom_vid_track.c_str(), &block))
        {
            vid_block_count_backward++;
        }

        while (pback.get_previous_data_block(k4a_cpp_ft_custom_subtitle_track.c_str(), &block))
        {
            subtitle_block_count_backward++;
        }

        ASSERT_GT(capture_count_backward, 0);
        ASSERT_GT(imu_count_backward, 0);
        ASSERT_EQ(imu_count_backward, capture_count_backward);
        ASSERT_EQ(capture_count_backward, vid_block_count_backward);
        ASSERT_EQ(capture_count_backward, subtitle_block_count_backward);
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

        while (pback.get_next_data_block(k4a_cpp_ft_custom_vid_track.c_str(), &block))
        {
            vid_block_count++;
            ASSERT_NE(block.get_device_timestamp_usec().count(), 0);
            ASSERT_NE(block.get_buffer_size(), (size_t)0);
            ASSERT_NE(block.get_buffer(), nullptr);
        }

        while (pback.get_next_data_block(k4a_cpp_ft_custom_subtitle_track.c_str(), &block))
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
    ASSERT_FALSE(pback.get_attachment(bad_attachment_name.c_str(), &data));
    ASSERT_TRUE(pback.get_attachment(k4a_cpp_ft_attachment.c_str(), &data));
    ASSERT_EQ(data.size(), k4a_cpp_ft_attachment.size());
    ASSERT_TRUE(memcmp(&data[0], k4a_cpp_ft_attachment.data(), data.size()) == 0);
}

TEST_F(k4a_cpp_ft, record_and_playback)
{
    test_record();
    test_playback();

    ASSERT_FALSE(std::remove(MKV_FILE_NAME.c_str()));
}
