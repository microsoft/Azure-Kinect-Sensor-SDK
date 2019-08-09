// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "firmware_helper.h"
#include <utcommon.h>

#include <k4ainternal/logging.h>
#include <k4ainternal/usbcommand.h>
#include <k4ainternal/calibration.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

class firmware_fw : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, TRACE_CALL(setup_common_test()));
        const ::testing::TestInfo *const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        printf("\nStarting test %s. This requires a connection exerciser to be connected.\n", test_info->name());
        LOG_INFO("Starting test %s.", test_info->name());
        LOG_INFO("Disconnecting the device");
        g_connection_exerciser->set_usb_port(0);
        ThreadAPI_Sleep(500);

        // Make sure that all of the firmwares have loaded correctly.
        ASSERT_TRUE(g_test_firmware_buffer != nullptr);
        ASSERT_TRUE(g_test_firmware_size > 0);
        ASSERT_TRUE(g_candidate_firmware_buffer != nullptr);
        ASSERT_TRUE(g_candidate_firmware_size > 0);
        ASSERT_TRUE(g_lkg_firmware_buffer != nullptr);
        ASSERT_TRUE(g_lkg_firmware_size > 0);
        ASSERT_TRUE(g_factory_firmware_buffer != nullptr);
        ASSERT_TRUE(g_factory_firmware_size > 0);

        // Make sure that the Test firmware has all components with a different version when compared to the Release
        // Candidate firmware.
        // Depth Sensor isn't expect to change.
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.audio, g_candidate_firmware_package_info.audio));
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.depth, g_candidate_firmware_package_info.depth));
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.rgb, g_candidate_firmware_package_info.rgb));

        // There should be no other devices as the tests use the default device to connect to.
        uint32_t device_count = 0;
        usb_cmd_get_device_count(&device_count);
        ASSERT_EQ((uint32_t)0, device_count);
    }

    void TearDown() override
    {
        if (firmware_handle != nullptr)
        {
            firmware_destroy(firmware_handle);
            firmware_handle = nullptr;
        }

        if (calibration_pre_update != nullptr)
        {
            free(calibration_pre_update);
            calibration_pre_update = nullptr;
            calibration_pre_update_size = 0;
        }

        if (calibration_post_update != nullptr)
        {
            free(calibration_post_update);
            calibration_post_update = nullptr;
            calibration_post_update_size = 0;
        }
    }

    k4a_result_t connect_device()
    {
        k4a_result_t result = TRACE_CALL(g_connection_exerciser->set_usb_port(g_k4a_port_number));
        ThreadAPI_Sleep(1000);

        if (K4A_SUCCEEDED(result))
        {
            int retry = 0;
            uint32_t device_count = 0;

            while (device_count == 0 && retry++ < 20)
            {
                ThreadAPI_Sleep(500);
                usb_cmd_get_device_count(&device_count);
            }

            if (device_count == 0 || retry >= 20)
            {
                result = K4A_RESULT_FAILED;
            }
        }

        return result;
    }

    k4a_result_t disconnect_device()
    {
        if (firmware_handle != nullptr)
        {
            firmware_destroy(firmware_handle);
            firmware_handle = nullptr;
        }

        k4a_result_t result = TRACE_CALL(g_connection_exerciser->set_usb_port(0));
        if (K4A_SUCCEEDED(result))
        {
            ThreadAPI_Sleep(1000);
        }

        return result;
    }

    bool compare_calibration()
    {
        if (calibration_pre_update_size == calibration_post_update_size &&
            0 == memcmp(calibration_pre_update, calibration_post_update, calibration_pre_update_size))
        {
            return true;
        }
        else
        {
            printf("Calibration pre and post update do not match!\n");
            printf("Calibration pre-update: %s\n", (char *)calibration_pre_update);
            printf("Calibration post-update: %s\n", (char *)calibration_post_update);
        }

        return false;
    }

    firmware_t firmware_handle = nullptr;

    uint8_t *calibration_pre_update = nullptr;
    size_t calibration_pre_update_size = 0;

    uint8_t *calibration_post_update = nullptr;
    size_t calibration_post_update_size = 0;
};

k4a_result_t read_calibration(uint8_t **calibration_data, size_t *calibration_size)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, calibration_data == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, calibration_size == NULL);

    if (*calibration_data != nullptr)
    {
        free(*calibration_data);
        *calibration_data = nullptr;
        *calibration_size = 0;
    }

    k4a_result_t result;
    depthmcu_t depth_handle = nullptr;
    calibration_t calibration_handle = nullptr;

    result = TRACE_CALL(depthmcu_create(K4A_DEVICE_DEFAULT, &depth_handle));

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(calibration_create(depth_handle, &calibration_handle));
    }

    if (K4A_SUCCEEDED(result))
    {
        if (K4A_BUFFER_RESULT_TOO_SMALL ==
            TRACE_BUFFER_CALL(calibration_get_raw_data(calibration_handle, nullptr, calibration_size)))
        {
            *calibration_data = (uint8_t *)malloc(*calibration_size);
        }
        else
        {
            result = K4A_RESULT_FAILED;
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (K4A_FAILED(
                TRACE_BUFFER_CALL(calibration_get_raw_data(calibration_handle, *calibration_data, calibration_size))))
        {
            result = K4A_RESULT_FAILED;
        }
    }

    if (calibration_handle != nullptr)
    {
        calibration_destroy(calibration_handle);
        calibration_handle = nullptr;
    }

    if (depth_handle != nullptr)
    {
        depthmcu_destroy(depth_handle);
        depth_handle = nullptr;
    }

    return result;
}

TEST_F(firmware_fw, DISABLED_update_timing)
{
    LOG_INFO("Beginning the manual test to get update timings.", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    printf("\n == Updating the device to the Candidate firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_candidate_firmware_buffer,
                                    g_candidate_firmware_size,
                                    g_candidate_firmware_package_info,
                                    true));

    printf("\n == Updating the device to the Test firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_test_firmware_buffer,
                                    g_test_firmware_size,
                                    g_test_firmware_package_info,
                                    true));
}

TEST_F(firmware_fw, simple_update_from_lkg)
{
    LOG_INFO("Beginning the basic update test from the LKG firmware.", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, read_calibration(&calibration_pre_update, &calibration_pre_update_size));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    printf("\n == Updating the device to the LKG firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_lkg_firmware_buffer,
                                    g_lkg_firmware_size,
                                    g_lkg_firmware_package_info,
                                    false));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, disconnect_device());
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, read_calibration(&calibration_post_update, &calibration_post_update_size));
    ASSERT_TRUE(compare_calibration());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    printf("\n == Updating the device to the Candidate firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_candidate_firmware_buffer,
                                    g_candidate_firmware_size,
                                    g_candidate_firmware_package_info,
                                    false));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, disconnect_device());
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, read_calibration(&calibration_post_update, &calibration_post_update_size));
    ASSERT_TRUE(compare_calibration());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    printf("\n == Updating the device to the Test firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_test_firmware_buffer,
                                    g_test_firmware_size,
                                    g_test_firmware_package_info,
                                    false));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, disconnect_device());
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, read_calibration(&calibration_post_update, &calibration_post_update_size));
    ASSERT_TRUE(compare_calibration());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));
}

TEST_F(firmware_fw, simple_update_from_factory)
{
    LOG_INFO("Beginning the basic update test from the Factory firmware.", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, read_calibration(&calibration_pre_update, &calibration_pre_update_size));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    printf("\n == Updating the device to the Factory firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_factory_firmware_buffer,
                                    g_factory_firmware_size,
                                    g_factory_firmware_package_info,
                                    false));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, disconnect_device());
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, read_calibration(&calibration_post_update, &calibration_post_update_size));
    ASSERT_TRUE(compare_calibration());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    printf("\n == Updating the device to the Candidate firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_candidate_firmware_buffer,
                                    g_candidate_firmware_size,
                                    g_candidate_firmware_package_info,
                                    false));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, disconnect_device());
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, read_calibration(&calibration_post_update, &calibration_post_update_size));
    ASSERT_TRUE(compare_calibration());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    printf("\n == Updating the device to the Test firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_test_firmware_buffer,
                                    g_test_firmware_size,
                                    g_test_firmware_package_info,
                                    false));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, disconnect_device());
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, connect_device());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, read_calibration(&calibration_post_update, &calibration_post_update_size));
    ASSERT_TRUE(compare_calibration());

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));
}
