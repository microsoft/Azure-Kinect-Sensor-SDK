// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

#include <k4ainternal/logging.h>
#include <ostream>

#ifdef _WIN32
#include <process.h>
#endif

using namespace testing;

// Define the output operator for k4a_result_t types for clean test output
std::ostream &operator<<(std::ostream &s, const k4a_result_t &val)
{
    switch (val)
    {
    case K4A_RESULT_SUCCEEDED:
        return s << "K4A_RESULT_SUCCEEDED";
    case K4A_RESULT_FAILED:
        return s << "K4A_RESULT_FAILED";
    }

    return s;
}

std::ostream &operator<<(std::ostream &s, const k4a_buffer_result_t &val)
{
    switch (val)
    {
    case K4A_BUFFER_RESULT_SUCCEEDED:
        return s << "K4A_BUFFER_RESULT_SUCCEEDED";
    case K4A_BUFFER_RESULT_FAILED:
        return s << "K4A_BUFFER_RESULT_FAILED";
    case K4A_BUFFER_RESULT_TOO_SMALL:
        return s << "K4A_BUFFER_RESULT_TOO_SMALL";
    }

    return s;
}

std::ostream &operator<<(std::ostream &s, const k4a_wait_result_t &val)
{
    switch (val)
    {
    case K4A_WAIT_RESULT_SUCCEEDED:
        return s << "K4A_BUFFER_RESULT_SUCCEEDED";
    case K4A_WAIT_RESULT_FAILED:
        return s << "K4A_WAIT_RESULT_FAILED";
    case K4A_WAIT_RESULT_TIMEOUT:
        return s << "K4A_WAIT_RESULT_TIMEOUT";
    }

    return s;
}

extern "C" {

static logger_t g_logger_handle = NULL;

static void enable_leak_detection()
{
#if K4A_ENABLE_LEAK_DETECTION
    // Enable this block for memory leak messaging to be send to STDOUT and debugger. This is useful for running a
    // script to run all tests and quickly review all output. Also enable EXE's for verification with application
    // verifier to to get call stacks of memory allocation. With memory leak addresses in hand use WinDbg command '!heap
    // -p -a <Address>' to get the stack
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

void k4a_unittest_init()
{
    enable_leak_detection();

    logger_config_t logger_config;
    logger_config_init_default(&logger_config);
    logger_config.env_var_log_to_a_file = "K4A_ENABLE_LOG_TO_A_FILE_TEST";
    assert(g_logger_handle == NULL);
    (void)logger_create(&logger_config, &g_logger_handle);

    // Mocked functions that return k4a_result_t should default to returning K4A_RESULT_FAILED
    // unless specifically expected
    DefaultValue<k4a_result_t>::Set(K4A_RESULT_FAILED);
    DefaultValue<k4a_wait_result_t>::Set(K4A_WAIT_RESULT_FAILED);
    DefaultValue<k4a_buffer_result_t>::Set(K4A_BUFFER_RESULT_FAILED);
}

void k4a_unittest_deinit()
{
    DefaultValue<k4a_result_t>::Clear();
    DefaultValue<k4a_wait_result_t>::Clear();
    DefaultValue<k4a_buffer_result_t>::Clear();
    logger_destroy(g_logger_handle);
}

#ifdef _WIN32
char g_log_file_name[1024];
void k4a_unittest_init_logging_with_processid()
{
    enable_leak_detection();

    logger_config_t logger_config;
    logger_t logger_handle = NULL;
    logger_config_init_default(&logger_config);

    // NOTE: K4A_ENABLE_LOG_TO_A_FILE=1 is what is needed to use this costom log
    snprintf(g_log_file_name, sizeof(g_log_file_name), "k4a_%d_0x%x.log", _getpid(), _getpid());
    printf("the log file name is %s\n", g_log_file_name);
    logger_config.log_file = g_log_file_name;

    (void)logger_create(&logger_config, &logger_handle);

    // Mocked functions that return k4a_result_t should default to returning K4A_RESULT_FAILED
    // unless specifically expected
    DefaultValue<k4a_result_t>::Set(K4A_RESULT_FAILED);
    DefaultValue<k4a_wait_result_t>::Set(K4A_WAIT_RESULT_FAILED);
    DefaultValue<k4a_buffer_result_t>::Set(K4A_BUFFER_RESULT_FAILED);
}
#endif

int k4a_test_commmon_main(int argc, char **argv)
{
#if 0
    k4a_unittest_init_logging_with_processid();
#else
    k4a_unittest_init();
#endif

    ::testing::InitGoogleTest(&argc, argv);

    int ret = RUN_ALL_TESTS();

    k4a_unittest_deinit();

    return ret;
}
}
