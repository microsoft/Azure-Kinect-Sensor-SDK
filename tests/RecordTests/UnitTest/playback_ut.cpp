#include <utcommon.h>
#include <k4a/k4a.h>

// Module being tested
#include <k4arecord/playback.h>

using namespace testing;

class playback_ut : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Disabled for now because the TestData file doesn't exist when using msbuild
TEST_F(playback_ut, DISABLED_open_basic_file)
{
    k4a_playback_t handle;
    k4a_result_t result = k4a_playback_open("TestData/playback_test_input.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // TODO: Check these cases in the unit test
    // std::cout << "K4A_COLOR_MODE: " << k4a_playback_get_tag(handle, "K4A_COLOR_MODE") << std::endl;
    // std::cout << "K4A_DEPTH_MODE: " << k4a_playback_get_tag(handle, "K4A_DEPTH_MODE") << std::endl;
    // std::cout << "Missing tag: " << k4a_playback_get_tag(handle, "FOO") << std::endl;
    /*
    std::cout << "Color track by name: " << get_track_by_name(context, "COLOR") << std::endl;
    std::cout << "Color track by tag: " << get_track_by_tag(context, "K4A_COLOR_MODE") << std::endl;

    std::cout << "Depth track by name: " << get_track_by_name(context, "DEPTH") << std::endl;
    std::cout << "Depth track by tag: " << get_track_by_tag(context, "K4A_DEPTH_MODE") << std::endl;

    std::cout << "missing track: " << get_track_by_tag(context, "foo") << std::endl;
    std::cout << "missing attachment: " << get_attachment_by_tag(context, "foo") << std::endl;

    std::cout << "'calibration.json' by name: " << get_attachment_by_name(context, "calibration.json") << std::endl;
    std::cout << "'calibration.json' by tag: " << get_attachment_by_tag(context, "K4A_CALIBRATION_FILE") << std::endl;
    */

    k4a_playback_close(handle);
}

TEST_F(playback_ut, DISABLED_open_full_file)
{
    k4a_playback_t handle;
    k4a_result_t result = k4a_playback_open("test.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    uint8_t buffer[8096];
    size_t buffer_size = 8096;
    k4a_buffer_result_t buffer_result = k4a_playback_get_raw_calibration(handle, &buffer[0], &buffer_size);
    ASSERT_EQ(buffer_result, K4A_BUFFER_RESULT_SUCCEEDED);

    k4a_calibration_t calibration;
    result = k4a_playback_get_calibration(handle, &calibration);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    result = k4a_playback_get_calibration(handle, &calibration);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    std::cout << "Previous capture" << std::endl;
    k4a_capture_t capture = NULL;
    k4a_stream_result_t playback_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(playback_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, nullptr);

    std::cout << "Next capture x10" << std::endl;
    for (int i = 0; i < 10; i++)
    {
        playback_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(playback_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_NE(capture, nullptr);
        k4a_capture_release(capture);
    }
    std::cout << "Previous capture x10" << std::endl;
    for (int i = 0; i < 9; i++)
    {
        playback_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(playback_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_NE(capture, nullptr);
        k4a_capture_release(capture);
    }
    playback_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(playback_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, nullptr);

    std::cout << "Seek to 1.1s" << std::endl;
    result = k4a_playback_seek_timestamp(handle, 1100000, K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    std::cout << "Next Capture x100" << std::endl;
    for (int i = 0; i < 100; i++)
    {
        playback_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_NE(result, K4A_STREAM_RESULT_FAILED);
        if (playback_result == K4A_STREAM_RESULT_EOF)
        {
            ASSERT_EQ(capture, nullptr);
            for (int j = i - 1; j >= 0; j--)
            {
                playback_result = k4a_playback_get_previous_capture(handle, &capture);
                ASSERT_EQ(result, K4A_STREAM_RESULT_SUCCEEDED);
                ASSERT_NE(capture, nullptr);
                k4a_capture_release(capture);
            }
            break;
        }
        else
        {
            ASSERT_NE(capture, nullptr);
            k4a_capture_release(capture);
        }
    }

    std::cout << "Seek to start" << std::endl;
    result = k4a_playback_seek_timestamp(handle, 0, K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    std::cout << "Next Capture x100" << std::endl;
    for (int i = 0; i < 100; i++)
    {
        playback_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_NE(result, K4A_STREAM_RESULT_FAILED);
        if (playback_result == K4A_STREAM_RESULT_EOF)
        {
            ASSERT_EQ(capture, nullptr);
            for (int j = i - 1; j >= 0; j--)
            {
                playback_result = k4a_playback_get_previous_capture(handle, &capture);
                ASSERT_EQ(result, K4A_STREAM_RESULT_SUCCEEDED);
                ASSERT_NE(capture, nullptr);
                k4a_capture_release(capture);
            }
            break;
        }
        else
        {
            ASSERT_NE(capture, nullptr);
            k4a_capture_release(capture);
        }
    }

    k4a_playback_close(handle);
}

int main(int argc, char **argv)
{
    k4a_unittest_init();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
