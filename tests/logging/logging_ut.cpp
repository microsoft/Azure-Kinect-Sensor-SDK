// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

// Module being tested
#include <k4ainternal/common.h>
#include <k4ainternal/logging.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

using namespace testing;

class logging_ut : public ::testing::Test
{
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(logging_ut, create)
{
    // Create the logging instance
    logger_t logger_handle1 = nullptr;
    logger_t logger_handle2 = nullptr;
    logger_config_t config;

    logger_config_init_default(&config);

    // Validate input checking
    ASSERT_EQ(K4A_RESULT_FAILED, logger_create(nullptr, nullptr));
    ASSERT_EQ(K4A_RESULT_FAILED, logger_create(&config, nullptr));
    ASSERT_EQ(K4A_RESULT_FAILED, logger_create(nullptr, &logger_handle1));
    ASSERT_EQ(logger_handle1, (logger_t) nullptr);

    // Create an instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, logger_create(&config, &logger_handle1));
    ASSERT_NE(logger_handle1, (logger_t) nullptr);

    // Create a second instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, logger_create(&config, &logger_handle2));
    ASSERT_NE(logger_handle2, (logger_t) nullptr);
    ASSERT_NE(logger_handle2, logger_handle1);

    // Verify the instances are unique
    ASSERT_NE(logger_handle1, logger_handle2);

    LOG_TRACE("Test Trace Message", 0);
    LOG_INFO("Test Info Message", 0);
    LOG_WARNING("Test Warning Message", 0);
    LOG_ERROR("Test Error Message", 0);
    LOG_CRITICAL("Test Critical Message", 0);

    logger_destroy(logger_handle1);
    logger_destroy(logger_handle2);
}

typedef struct _logger_test_callback_info_t
{
    int message_count_trace;
    int message_count_info;
    int message_count_warning;
    int message_count_error;
    int message_count_critical;
} logger_test_callback_info_t;

k4a_logging_message_cb_t logging_callback_function;
k4a_logging_message_cb_t logging_callback_function_not_used;

void logging_callback_function(void *context, k4a_log_level_t level, const char *file, int line, const char *message)
{
    ASSERT_TRUE(level >= K4A_LOG_LEVEL_CRITICAL && level <= K4A_LOG_LEVEL_TRACE);
    ASSERT_TRUE(context != nullptr);

    (void)file;
    (void)line;
    (void)message;

    logger_test_callback_info_t *info = (logger_test_callback_info_t *)context;
    switch (level)
    {
    case K4A_LOG_LEVEL_CRITICAL:
        info->message_count_critical++;
        break;
    case K4A_LOG_LEVEL_ERROR:
        info->message_count_error++;
        break;
    case K4A_LOG_LEVEL_WARNING:
        info->message_count_warning++;
        break;
    case K4A_LOG_LEVEL_INFO:
        info->message_count_info++;
        break;
    case K4A_LOG_LEVEL_TRACE:
    default:
        info->message_count_trace++;
        break;
    }
}

void logging_callback_function_not_used(void *context,
                                        k4a_log_level_t level,
                                        const char *file,
                                        int line,
                                        const char *message)
{
    ASSERT_TRUE(0);

    (void)context;
    (void)level;
    (void)file;
    (void)line;
    (void)message;
}

TEST_F(logging_ut, callback)
{
    logger_test_callback_info_t info = { 0 };

    // Validate input checking
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, logger_register_message_callback(nullptr, nullptr, (k4a_log_level_t)-1));
    ASSERT_EQ(K4A_RESULT_FAILED,
              logger_register_message_callback(logging_callback_function, &info, (k4a_log_level_t)-1));
    ASSERT_EQ(K4A_RESULT_FAILED,
              logger_register_message_callback(logging_callback_function, &info, (k4a_log_level_t)100));

    // successful register with no context
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              logger_register_message_callback(logging_callback_function, nullptr, K4A_LOG_LEVEL_TRACE));
    // successful unregister
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, logger_register_message_callback(nullptr, nullptr, (k4a_log_level_t)-1));
    // successful register
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              logger_register_message_callback(logging_callback_function, &info, K4A_LOG_LEVEL_TRACE));

    // 2nd registration should fail
    ASSERT_EQ(K4A_RESULT_FAILED,
              logger_register_message_callback(logging_callback_function_not_used, &info, K4A_LOG_LEVEL_INFO));

    {
        memset(&info, 0, sizeof(info));
        LOG_TRACE("Test Trace Message", 0);
        LOG_INFO("Test Info Message", 0);
        LOG_WARNING("Test Warning Message", 0);
        LOG_ERROR("Test Error Message", 0);
        LOG_CRITICAL("Test Critical Message", 0);

        ASSERT_EQ(info.message_count_critical, 1);
        ASSERT_EQ(info.message_count_error, 1);
        ASSERT_EQ(info.message_count_warning, 1);
        ASSERT_EQ(info.message_count_info, 1);
        ASSERT_EQ(info.message_count_trace, 1);
    }

    // re-registration (same function ptr) should pass and update level
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              logger_register_message_callback(logging_callback_function, &info, K4A_LOG_LEVEL_ERROR));

    {
        memset(&info, 0, sizeof(info));
        LOG_TRACE("Test Trace Message", 0);
        LOG_INFO("Test Info Message", 0);
        LOG_WARNING("Test Warning Message", 0);
        LOG_ERROR("Test Error Message", 0);
        LOG_CRITICAL("Test Critical Message", 0);

        ASSERT_EQ(info.message_count_critical, 1);
        ASSERT_EQ(info.message_count_error, 1);
        ASSERT_EQ(info.message_count_warning, 0);
        ASSERT_EQ(info.message_count_info, 0);
        ASSERT_EQ(info.message_count_trace, 0);
    }

    // re-registration (same function ptr) should pass and update level
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              logger_register_message_callback(logging_callback_function, &info, K4A_LOG_LEVEL_OFF));

    {
        memset(&info, 0, sizeof(info));
        LOG_TRACE("Test Trace Message", 0);
        LOG_INFO("Test Info Message", 0);
        LOG_WARNING("Test Warning Message", 0);
        LOG_ERROR("Test Error Message", 0);
        LOG_CRITICAL("Test Critical Message", 0);

        ASSERT_EQ(info.message_count_critical, 0);
        ASSERT_EQ(info.message_count_error, 0);
        ASSERT_EQ(info.message_count_warning, 0);
        ASSERT_EQ(info.message_count_info, 0);
        ASSERT_EQ(info.message_count_trace, 0);
    }

    // multiple calls to clear the callback function
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, logger_register_message_callback(nullptr, &info, K4A_LOG_LEVEL_ERROR));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, logger_register_message_callback(nullptr, &info, K4A_LOG_LEVEL_ERROR));

    // registration should pass
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              logger_register_message_callback(logging_callback_function, &info, K4A_LOG_LEVEL_INFO));

    {
        memset(&info, 0, sizeof(info));
        LOG_TRACE("Test Trace Message", 0);
        LOG_INFO("Test Info Message", 0);
        LOG_WARNING("Test Warning Message", 0);
        LOG_ERROR("Test Error Message", 0);
        LOG_CRITICAL("Test Critical Message", 0);

        ASSERT_EQ(info.message_count_critical, 1);
        ASSERT_EQ(info.message_count_error, 1);
        ASSERT_EQ(info.message_count_warning, 1);
        ASSERT_EQ(info.message_count_info, 1);
        ASSERT_EQ(info.message_count_trace, 0);
    }
    ASSERT_EQ(K4A_RESULT_FAILED,
              logger_register_message_callback(logging_callback_function_not_used, &info, K4A_LOG_LEVEL_ERROR));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, logger_register_message_callback(NULL, NULL, K4A_LOG_LEVEL_ERROR));
}

#define TEST_RETURN_VALUE (22)

typedef struct _logger_callback_threading_test_data_t
{
    volatile int done;
    LOCK_HANDLE lock;
} logger_callback_threading_test_data_t;

static int logger_callback_thread(void *param)
{
    logger_callback_threading_test_data_t *data = (logger_callback_threading_test_data_t *)param;

    Lock(data->lock);
    do
    {
        LOG_TRACE("Test Trace Message", 0);
        LOG_INFO("Test Info Message", 0);
        LOG_WARNING("Test Warning Message", 0);
        LOG_ERROR("Test Error Message", 0);
        LOG_CRITICAL("Test Critical Message", 0);
    } while (data->done == 0);
    return TEST_RETURN_VALUE;
}

TEST_F(logging_ut, callback_threading)
{
    THREAD_HANDLE th;
    TICK_COUNTER_HANDLE tick;
    tickcounter_ms_t start_time_ms, now;
    logger_callback_threading_test_data_t data = { 0 };
    logger_test_callback_info_t info = { 0 };
    int count = 1;

    ASSERT_NE((data.lock = Lock_Init()), (LOCK_HANDLE)NULL);

    ASSERT_NE((TICK_COUNTER_HANDLE)NULL, tick = tickcounter_create());

    // prevent the threads from running
    Lock(data.lock);

    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th, logger_callback_thread, &data));

    ThreadAPI_Sleep(100); // Let the thread get started.

    // start the test
    Unlock(data.lock);

    ASSERT_EQ(0, tickcounter_get_current_ms(tick, &start_time_ms));
    do
    {
        // Loop registering and unregistering a callback function while another thread continuous writes messages.
        ThreadAPI_Sleep(20);
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  logger_register_message_callback(logging_callback_function, &info, K4A_LOG_LEVEL_TRACE))
            << " the count is " << count << "\n";

        ThreadAPI_Sleep(20);
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, logger_register_message_callback(NULL, NULL, K4A_LOG_LEVEL_TRACE))
            << " the count is " << count << "\n";

        ASSERT_EQ(0, tickcounter_get_current_ms(tick, &now));
        count++;
    } while (now - start_time_ms < 5000);

    data.done = true;

    // Wait for the thread to terminate
    int result;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th, &result));

    ASSERT_EQ(result, TEST_RETURN_VALUE);

    // Verify all our allocations were released
    Lock_Deinit(data.lock);
    tickcounter_destroy(tick);
}

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}
