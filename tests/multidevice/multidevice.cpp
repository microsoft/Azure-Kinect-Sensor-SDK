// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>
#include <utcommon.h>

#include <gtest/gtest.h>

#include <k4ainternal/capturesync.h>
#include <k4ainternal/capture.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/condition.h>

// This wait is effectively an infinite wait, setting to 5 min will prevent the test from blocking indefinitely in the
// event the test regresses.
#define WAIT_TEST_INFINITE (5 * 60 * 1000)

#define LLD(val) ((int64_t)(val))
#define NULL_IMAGE 0
#define NULL_DEVICE 0

// How close 2 timestamps should be to be considered accurately synchronized.
const int MAX_SYNC_CAPTURE_DIFFERENCE_USEC = 100;

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}

class multidevice_ft : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        ASSERT_EQ(m_device1, nullptr);
        ASSERT_EQ(m_device2, nullptr);
    }

    virtual void TearDown()
    {
        if (m_device1 != nullptr)
        {
            k4a_device_close(m_device1);
            m_device1 = nullptr;
        }
        if (m_device2 != nullptr)
        {
            k4a_device_close(m_device2);
            m_device2 = nullptr;
        }
    }

    k4a_device_t m_device1 = nullptr;
    k4a_device_t m_device2 = nullptr;
};

class multidevice_sync_ft : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        srand((unsigned int)time(0)); // use current time as seed for random generator
    }

    virtual void TearDown() {}
};

TEST_F(multidevice_ft, open_close_two)
{
    ASSERT_LE((uint32_t)2, k4a_device_get_installed_count());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(0, &m_device1));
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_open(0, &m_device1));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(1, &m_device2));
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_open(1, &m_device2));
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_open(2, &m_device2));
    k4a_device_close(m_device1);
    m_device1 = NULL;
    k4a_device_close(m_device2);
    m_device2 = NULL;

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(1, &m_device1));
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_open(1, &m_device1));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(0, &m_device2));
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_open(0, &m_device2));
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_open(2, &m_device2));
    k4a_device_close(m_device1);
    m_device1 = NULL;
    k4a_device_close(m_device2);
    m_device2 = NULL;
}

TEST_F(multidevice_ft, stream_two_1_then_2)
{
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    ASSERT_LE((uint32_t)2, k4a_device_get_installed_count());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(0, &m_device1));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(1, &m_device2));
    ASSERT_NE(m_device1, m_device2);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device1, &config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device2, &config));

    for (int image_count = 0; image_count < 25; image_count++)
    {
        k4a_capture_t capture1 = NULL;
        k4a_capture_t capture2 = NULL;
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device1, &capture1, WAIT_TEST_INFINITE))
            << "iteration was " << image_count << "\n";
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device2, &capture2, WAIT_TEST_INFINITE))
            << "iteration was " << image_count << "\n";
        if (capture1)
        {
            k4a_capture_release(capture1);
        }
        if (capture2)
        {
            k4a_capture_release(capture2);
        }
    }

    k4a_device_close(m_device1);
    m_device1 = NULL;
    k4a_device_close(m_device2);
    m_device2 = NULL;
}

TEST_F(multidevice_ft, stream_two_2_then_1)
{
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    ASSERT_LE((uint32_t)2, k4a_device_get_installed_count());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(1, &m_device2));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(0, &m_device1));
    ASSERT_NE(m_device1, m_device2);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device2, &config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device1, &config));

    for (int image_count = 0; image_count < 25; image_count++)
    {
        k4a_capture_t capture1 = NULL;
        k4a_capture_t capture2 = NULL;
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device2, &capture2, WAIT_TEST_INFINITE))
            << "iteration was " << image_count << "\n";
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device1, &capture1, WAIT_TEST_INFINITE))
            << "iteration was " << image_count << "\n";
        if (capture2)
        {
            k4a_capture_release(capture2);
        }
        if (capture1)
        {
            k4a_capture_release(capture1);
        }
    }

    k4a_device_close(m_device2);
    m_device2 = NULL;
    k4a_device_close(m_device1);
    m_device1 = NULL;
}

#define RETURN_K4A_RESULT_LE(msg1, msg2, v1, v2)                                                                       \
    if (!(v1 <= v2))                                                                                                   \
    {                                                                                                                  \
        printf("%s(%d): ERROR: expected %s <= %s\n %" PRId64 " vs %" PRId64 "\n",                                      \
               __FILE__,                                                                                               \
               __LINE__,                                                                                               \
               msg1,                                                                                                   \
               msg2,                                                                                                   \
               LLD(v1),                                                                                                \
               LLD(v2));                                                                                               \
        return K4A_RESULT_FAILED;                                                                                      \
    }

#define RETURN_K4A_RESULT_EQ(msg1, msg2, v1, v2)                                                                       \
    if (!(v1 == v2))                                                                                                   \
    {                                                                                                                  \
        printf("%s(%d): ERROR: expected %s == %s\n %" PRId64 " vs %" PRId64 "\n",                                      \
               __FILE__,                                                                                               \
               __LINE__,                                                                                               \
               msg1,                                                                                                   \
               msg2,                                                                                                   \
               LLD(v1),                                                                                                \
               LLD(v2));                                                                                               \
        return K4A_RESULT_FAILED;                                                                                      \
    }

#define RETURN_K4A_RESULT_NE(msg1, msg2, v1, v2)                                                                       \
    if (!(v1 != v2))                                                                                                   \
    {                                                                                                                  \
        printf("%s(%d): ERROR: expected %s != %s\n %" PRId64 " vs %" PRId64 "\n",                                      \
               __FILE__,                                                                                               \
               __LINE__,                                                                                               \
               msg1,                                                                                                   \
               msg2,                                                                                                   \
               LLD(v1),                                                                                                \
               LLD(v2));                                                                                               \
        return K4A_RESULT_FAILED;                                                                                      \
    }

#define R_EXPECT_LE(v1, v2) RETURN_K4A_RESULT_LE(#v1, #v2, v1, v2)
#define R_EXPECT_EQ(v1, v2) RETURN_K4A_RESULT_EQ(#v1, #v2, v1, v2)
#define R_EXPECT_NE(v1, v2) RETURN_K4A_RESULT_NE(#v1, #v2, v1, v2)

static k4a_result_t open_master_and_subordinate(k4a_device_t *master, k4a_device_t *subordinate)
{
    *master = NULL;
    *subordinate = NULL;

    uint32_t devices_present = k4a_device_get_installed_count();
    R_EXPECT_LE((int64_t)2, devices_present);

    for (uint32_t x = 0; x < devices_present; x++)
    {
        k4a_device_t device;
        R_EXPECT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(x, &device));

        bool sync_in_cable_present;
        bool sync_out_cable_present;

        R_EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                    k4a_device_get_sync_jack(device, &sync_in_cable_present, &sync_out_cable_present));

        if (*master == NULL && sync_out_cable_present)
        {
            *master = device;
            device = NULL;
        }

        if (*subordinate == NULL && sync_in_cable_present)
        {
            *subordinate = device;
            device = NULL;
        }

        if (device)
        {
            k4a_device_close(device);
        }
    }

    R_EXPECT_NE(NULL_DEVICE, *master);
    R_EXPECT_NE(NULL_DEVICE, *subordinate);
    return K4A_RESULT_SUCCEEDED;
}

static k4a_result_t set_power_and_exposure(k4a_device_t device, int exposure_setting, int power_line_setting)
{
    int read_power_line_setting;
    int read_exposure;
    k4a_color_control_mode_t read_mode;

    R_EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                k4a_device_set_color_control(device,
                                             K4A_COLOR_CONTROL_POWERLINE_FREQUENCY,
                                             K4A_COLOR_CONTROL_MODE_MANUAL,
                                             power_line_setting));

    R_EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                k4a_device_get_color_control(device,
                                             K4A_COLOR_CONTROL_POWERLINE_FREQUENCY,
                                             &read_mode,
                                             &read_power_line_setting));
    R_EXPECT_EQ(read_power_line_setting, power_line_setting);

    R_EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                k4a_device_set_color_control(device,
                                             K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                             K4A_COLOR_CONTROL_MODE_MANUAL,
                                             (int32_t)exposure_setting));
    R_EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                k4a_device_get_color_control(device,
                                             K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                             &read_mode,
                                             &read_exposure));
    R_EXPECT_EQ(exposure_setting, read_exposure);
    return K4A_RESULT_SUCCEEDED;
}

static k4a_result_t get_syncd_captures(k4a_device_t master,
                                       k4a_device_t sub,
                                       k4a_capture_t *cap_m,
                                       k4a_capture_t *cap_s,
                                       uint32_t subordinate_delay_off_master_usec)
{
    const int timeout_ms = 10000;
    int64_t ts_m, ts_s, ts_s_adj;
    k4a_image_t image_m, image_s;
    int tries = 0;

    R_EXPECT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(master, cap_m, timeout_ms));
    R_EXPECT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(sub, cap_s, timeout_ms));

    R_EXPECT_NE(NULL_IMAGE, (image_m = k4a_capture_get_color_image(*cap_m)));
    R_EXPECT_NE(NULL_IMAGE, (image_s = k4a_capture_get_color_image(*cap_s)));
    ts_m = (int64_t)k4a_image_get_device_timestamp_usec(image_m);
    ts_s = (int64_t)k4a_image_get_device_timestamp_usec(image_s);
    k4a_image_release(image_m);
    k4a_image_release(image_s);

    ts_s_adj = ts_s - subordinate_delay_off_master_usec;

    int64_t ts_delta = std::abs(ts_m - ts_s_adj);
    while (ts_delta > MAX_SYNC_CAPTURE_DIFFERENCE_USEC)
    {
        // bail out if it never happens
        R_EXPECT_LE(tries++, 100);

        if (ts_m < ts_s_adj)
        {
            printf("Master too old m:%9" PRId64 " s:%9" PRId64 " adj sub:%9" PRId64 " adj delta:%9" PRId64 "\n",
                   ts_m,
                   ts_s,
                   ts_s_adj,
                   ts_delta);
            k4a_capture_release(*cap_m);
            R_EXPECT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(master, cap_m, 10000));
            R_EXPECT_NE(NULL_IMAGE, (image_m = k4a_capture_get_color_image(*cap_m)));
            ts_m = (int64_t)k4a_image_get_device_timestamp_usec(image_m);
            k4a_image_release(image_m);
        }
        else
        {
            printf("Sub    too old m:%9" PRId64 " s:%9" PRId64 " adj sub:%9" PRId64 " adj delta:%9" PRId64 "\n",
                   ts_m,
                   ts_s,
                   ts_s_adj,
                   ts_delta);
            k4a_capture_release(*cap_s);
            R_EXPECT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(sub, cap_s, 10000));
            R_EXPECT_NE(NULL_IMAGE, (image_s = k4a_capture_get_color_image(*cap_s)));
            ts_s = (int64_t)k4a_image_get_device_timestamp_usec(image_s);
            ts_s_adj = ts_s - subordinate_delay_off_master_usec;
            k4a_image_release(image_s);
        }

        ts_delta = std::abs(ts_m - ts_s_adj);
    }
    return K4A_RESULT_SUCCEEDED;
}
static k4a_result_t verify_ts(int64_t ts_1, int64_t ts_2, int64_t ts_offset, const char *error_message)
{
    int64_t ts_1_adjust = ts_1 + ts_offset;
    int64_t ts_result = std::abs(ts_1_adjust - ts_2);
    if (ts_result > MAX_SYNC_CAPTURE_DIFFERENCE_USEC)
    {
        printf("    ERROR timestamps are not within range.\n    TS1 + TS_Offset should be ~= TS2. %s\n    ts1=%" PRId64
               " "
               "ts2=%" PRId64 " ts_offset=%" PRId64 " diff=%" PRId64 "\n",
               error_message,
               ts_1,
               ts_2,
               ts_offset,
               ts_result);
        R_EXPECT_LE(ts_result, MAX_SYNC_CAPTURE_DIFFERENCE_USEC);
    }
    return K4A_RESULT_SUCCEEDED;
}

TEST_F(multidevice_sync_ft, multi_sync_validation)
{
    k4a_device_t master, subordinate;
    k4a_fps_t frame_rate = K4A_FRAMES_PER_SECOND_30;
    int32_t fps_in_usec = 1000000 / (int32_t)k4a_convert_fps_to_uint(frame_rate);

    ASSERT_EQ(open_master_and_subordinate(&master, &subordinate), K4A_RESULT_SUCCEEDED);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, set_power_and_exposure(master, 8330, 2));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, set_power_and_exposure(subordinate, 8330, 2));

    k4a_device_configuration_t default_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    default_config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    default_config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
    default_config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    default_config.camera_fps = frame_rate;
    default_config.subordinate_delay_off_master_usec = 0;
    default_config.depth_delay_off_color_usec = 0;
    default_config.synchronized_images_only = true;

    k4a_device_configuration_t s_config = default_config;
    s_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
    s_config.depth_delay_off_color_usec = (2 * fps_in_usec * rand() / RAND_MAX - fps_in_usec);
    s_config.subordinate_delay_off_master_usec = (uint32_t)(1 * fps_in_usec * rand() / RAND_MAX);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(subordinate, &s_config));

    k4a_device_configuration_t m_config = default_config;
    m_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
    m_config.depth_delay_off_color_usec = (2 * fps_in_usec * rand() / RAND_MAX - fps_in_usec);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(master, &m_config));

    printf("Test Running with the following settings:\n");
    printf("      Master depth_delay_off_color_usec: %d\n", m_config.depth_delay_off_color_usec);
    printf("         Sub depth_delay_off_color_usec: %d\n", s_config.depth_delay_off_color_usec);
    printf("  Sub subordinate_delay_off_master_usec: %d\n", s_config.subordinate_delay_off_master_usec);

    printf("\nDelta = Time off master color.\n");
    printf("Master Color, Master IR(Delta), Sub Color(Delta), Sub IR(Delta)\n");
    printf("---------------------------------------------------------------\n");
    for (int x = 0; x < 100; x++)
    {
        k4a_capture_t cap_m, cap_s;
        int64_t ts_m_c, ts_m_ir, ts_s_c, ts_s_ir;
        k4a_image_t image_c_m, image_ir_m, image_c_s, image_ir_s;

        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  get_syncd_captures(master, subordinate, &cap_m, &cap_s, s_config.subordinate_delay_off_master_usec));

        ASSERT_FALSE(NULL_IMAGE == (image_c_m = k4a_capture_get_color_image(cap_m)));
        ASSERT_FALSE(NULL_IMAGE == (image_c_s = k4a_capture_get_color_image(cap_s)));
        ASSERT_FALSE(NULL_IMAGE == (image_ir_m = k4a_capture_get_ir_image(cap_m)));
        ASSERT_FALSE(NULL_IMAGE == (image_ir_s = k4a_capture_get_ir_image(cap_s)));

        ts_m_c = (int64_t)k4a_image_get_device_timestamp_usec(image_c_m);
        ts_s_c = (int64_t)k4a_image_get_device_timestamp_usec(image_c_s);
        ts_m_ir = (int64_t)k4a_image_get_device_timestamp_usec(image_ir_m);
        ts_s_ir = (int64_t)k4a_image_get_device_timestamp_usec(image_ir_s);

        printf("%9" PRId64 ", %9" PRId64 "(%5" PRId64 "), %9" PRId64 "(%5" PRId64 "), %9" PRId64 "(%5" PRId64 ")\n",
               ts_m_c,
               ts_m_ir,
               ts_m_ir - ts_m_c,
               ts_s_c,
               ts_s_c - ts_m_c,
               ts_s_ir,
               ts_s_ir - ts_m_c);

        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  verify_ts(ts_m_c,
                            ts_m_ir,
                            m_config.depth_delay_off_color_usec,
                            "TS1 is Master Color, TS2 is Master Ir"));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  verify_ts(ts_s_c,
                            ts_s_ir,
                            s_config.depth_delay_off_color_usec,
                            "TS1 is Subordinate Color, TS2 is Subordinate Ir"));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  verify_ts(ts_m_c,
                            ts_s_c,
                            (int64_t)s_config.subordinate_delay_off_master_usec,
                            "TS1 is Master Color, TS2 is Subordinate Color"));

        k4a_image_release(image_c_m);
        k4a_image_release(image_c_s);
        k4a_image_release(image_ir_m);
        k4a_image_release(image_ir_s);

        k4a_capture_release(cap_m);
        k4a_capture_release(cap_s);
    }
    k4a_device_close(master);
    k4a_device_close(subordinate);
}

TEST_F(multidevice_ft, ensure_color_camera_is_enabled)
{
    bool master_device_found = false;
    bool subordinate_device_found = false;
    uint32_t devices_present = k4a_device_get_installed_count();
    ASSERT_LE((uint32_t)2, devices_present);

    for (uint32_t x = 0; x < devices_present; x++)
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(x, &m_device1));

        bool sync_in_cable_present;
        bool sync_out_cable_present;

        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

        config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
        config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
        config.camera_fps = K4A_FRAMES_PER_SECOND_30;

        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_get_sync_jack(m_device1, &sync_in_cable_present, &sync_out_cable_present));

        if (sync_out_cable_present)
        {
            // Negative test
            config.wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
            ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_start_cameras(m_device1, &config));
            k4a_device_stop_cameras(m_device1);

            // Positive Test
            config.wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device1, &config));
            k4a_device_stop_cameras(m_device1);

            master_device_found = true;
        }

        if (sync_in_cable_present)
        {
            // Positive Test
            config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device1, &config));
            k4a_device_stop_cameras(m_device1);

            // Positive Test
            config.wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device1, &config));
            k4a_device_stop_cameras(m_device1);

            subordinate_device_found = true;
        }

        if (subordinate_device_found && sync_out_cable_present)
        {
            // Done with the test
            break;
        }

        k4a_device_close(m_device1);
        m_device1 = NULL;
    }

    // Make sure we found both devices.
    ASSERT_EQ(master_device_found, true);
    ASSERT_EQ(subordinate_device_found, true);
}

typedef struct _parallel_start_data_t
{
    k4a_device_t device;
    k4a_device_configuration_t *config;
    bool started;
    LOCK_HANDLE lock;
} parallel_start_data_t;

static int parallel_start_thread(void *param)
{
    parallel_start_data_t *data = (parallel_start_data_t *)param;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    Lock(data->lock);
    Unlock(data->lock);

    if (!data->started)
    {
        EXPECT_EQ(K4A_RESULT_SUCCEEDED, result = k4a_device_start_cameras(data->device, data->config));

        if (K4A_SUCCEEDED(result))
        {
            EXPECT_EQ(K4A_RESULT_SUCCEEDED, result = k4a_device_start_imu(data->device));
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        ThreadAPI_Sleep(1000);
    }

    if (data->device)
    {
        k4a_device_stop_cameras(data->device);
        k4a_device_stop_imu(data->device);
        k4a_device_close(data->device);
        data->device = NULL;
    }

    return result;
}

#ifdef _WIN32
TEST_F(multidevice_ft, start_parallel)
#else
// GitHub: #769 https://github.com/microsoft/Azure-Kinect-Sensor-SDK/issues/769
TEST_F(multidevice_ft, DISABLED_start_parallel)
#endif
{
    LOCK_HANDLE lock;
    THREAD_HANDLE th1, th2;
    ASSERT_NE((lock = Lock_Init()), (LOCK_HANDLE)NULL);
    parallel_start_data_t data1 = { 0 }, data2 = { 0 };

    ASSERT_EQ((uint32_t)2, k4a_device_get_installed_count());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(0, &data1.device));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(1, &data2.device));

    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    // prevent the threads from running
    Lock(lock);

    data1.lock = data2.lock = lock;
    data2.config = data1.config = &config;

    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th1, parallel_start_thread, &data1));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th2, parallel_start_thread, &data2));

    // start the test
    Unlock(lock);

    // Wait for the threads to terminate
    int result1, result2;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th1, &result1));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th2, &result2));

    ASSERT_EQ(result1, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(result2, K4A_RESULT_SUCCEEDED);

    if (data1.device)
    {
        k4a_device_close(data1.device);
    }

    if (data2.device)
    {
        k4a_device_close(data2.device);
    }

    Lock_Deinit(lock);
}

TEST_F(multidevice_ft, close_parallel)
{
    LOCK_HANDLE lock;
    THREAD_HANDLE th1, th2;
    ASSERT_NE((lock = Lock_Init()), (LOCK_HANDLE)NULL);
    parallel_start_data_t data1 = { 0 }, data2 = { 0 };

    ASSERT_EQ((uint32_t)2, k4a_device_get_installed_count());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(0, &data1.device));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(1, &data2.device));

    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    data2.config = data1.config = &config;
    data1.lock = data2.lock = lock;

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(data1.device, data1.config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(data2.device, data2.config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(data1.device));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(data2.device));

    data1.started = data2.started = true;

    // Prevent the threads from running
    Lock(lock);

    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th1, parallel_start_thread, &data1));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th2, parallel_start_thread, &data2));

    // start the test
    Unlock(lock);

    // Wait for the threads to terminate
    int result1, result2;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th1, &result1));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th2, &result2));

    ASSERT_EQ(result1, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(result2, K4A_RESULT_SUCCEEDED);

    if (data1.device)
    {
        k4a_device_close(data1.device);
    }

    if (data2.device)
    {
        k4a_device_close(data2.device);
    }

    Lock_Deinit(lock);
}

// TODO https://github.com/microsoft/Azure-Kinect-Sensor-SDK/issues/818
TEST_F(multidevice_sync_ft, DISABLED_multi_sync_no_color)
{
    k4a_device_t master, subordinate;
    k4a_fps_t frame_rate = K4A_FRAMES_PER_SECOND_30;

    ASSERT_EQ(open_master_and_subordinate(&master, &subordinate), K4A_RESULT_SUCCEEDED);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, set_power_and_exposure(master, 8330, 2));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, set_power_and_exposure(subordinate, 8330, 2));

    k4a_device_configuration_t default_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    default_config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    default_config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
    default_config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    default_config.camera_fps = frame_rate;
    default_config.subordinate_delay_off_master_usec = 0;
    default_config.depth_delay_off_color_usec = 0;
    default_config.synchronized_images_only = true;

    k4a_device_configuration_t s_config = default_config;
    s_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
    s_config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
    s_config.synchronized_images_only = false;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(subordinate, &s_config));

    k4a_device_configuration_t m_config = default_config;
    m_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(master, &m_config));

    for (int x = 0; x < 20; x++)
    {
        k4a_capture_t capture;
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(master, &capture, 10000));
        k4a_capture_release(capture);
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(subordinate, &capture, 10000));
        k4a_capture_release(capture);
    }

    k4a_device_stop_cameras(master);
    k4a_device_stop_cameras(subordinate);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(subordinate, &s_config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(master, &m_config));

    for (int x = 0; x < 20; x++)
    {
        k4a_capture_t capture;
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(master, &capture, 10000));
        k4a_capture_release(capture);
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(subordinate, &capture, 10000));
        k4a_capture_release(capture);
    }

    // Reverse the stop order from above and then start again to ensure all starts as expected.
    k4a_device_stop_cameras(subordinate);
    k4a_device_stop_cameras(master);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(subordinate, &s_config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(master, &m_config));

    for (int x = 0; x < 20; x++)
    {
        k4a_capture_t capture;
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(master, &capture, 10000));
        k4a_capture_release(capture);
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(subordinate, &capture, 10000));
        k4a_capture_release(capture);
    }

    // Close master first and make sure not hang or crashes.
    k4a_device_close(master);
    k4a_device_close(subordinate);
}
