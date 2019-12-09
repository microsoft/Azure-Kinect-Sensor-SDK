// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4arecord/record.hpp>
#include <k4arecord/playback.hpp>
#include <utcommon.h>

#include <gtest/gtest.h>

const std::string MKV_MJPEG_FILE_NAME("./recorded_with_MJPEG.mkv");
const std::string MKV_BGRA32_FILE_NAME("./recorded_with_BGRA32.mkv");

using namespace k4a;

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}

class k4a_cpp_ft : public ::testing::Test
{
public:
    virtual void SetUp() {}

    virtual void TearDown()
    {
        std::remove(MKV_MJPEG_FILE_NAME.c_str());
        std::remove(MKV_BGRA32_FILE_NAME.c_str());
    }
};

static void create_recorded_file(k4a_image_format_t format, const std::string &file_name)
{
    record recorder;
    device kinect = device::open(0);
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.color_format = format;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.synchronized_images_only = true;
    kinect.start_cameras(&config);

    recorder = record::create(file_name.c_str(), kinect, config);
    ASSERT_TRUE(recorder);

    recorder.write_header();

    for (int x = 0; x < 33; x++)
    {
        capture capture;
        if (kinect.get_capture(&capture, std::chrono::milliseconds(1000)))
        {
            recorder.write_capture(capture);
            capture.reset();
        }
    }

    recorder.flush();
    kinect.stop_cameras();
}

static void test_playback(const std::string &file_name)
{
    playback pb = playback::open(file_name.c_str());
    ASSERT_TRUE(pb.is_valid());

    k4a_record_configuration_t config = pb.get_record_configuration();
    ASSERT_EQ(K4A_IMAGE_FORMAT_COLOR_MJPG, config.color_format) << "Testing color format for " << file_name << "\n";
    ASSERT_EQ(K4A_DEPTH_MODE_NFOV_UNBINNED, config.depth_mode) << "Testing depth mode for " << file_name << "\n";

    pb.set_color_conversion(K4A_IMAGE_FORMAT_COLOR_BGRA32);

    capture cap;
    while (pb.get_next_capture(&cap))
    {
        image color = cap.get_color_image();
        ASSERT_EQ(color.get_format(), K4A_IMAGE_FORMAT_COLOR_BGRA32)
            << "Testing capture format for " << file_name << "\n";
        color.reset();
        cap.reset();
    }
}

TEST_F(k4a_cpp_ft, record_and_playback)
{
    create_recorded_file(K4A_IMAGE_FORMAT_COLOR_MJPG, MKV_MJPEG_FILE_NAME);
    create_recorded_file(K4A_IMAGE_FORMAT_COLOR_BGRA32, MKV_BGRA32_FILE_NAME);

    test_playback(MKV_MJPEG_FILE_NAME);
    test_playback(MKV_BGRA32_FILE_NAME);
}
