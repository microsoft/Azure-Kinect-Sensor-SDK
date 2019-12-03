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

char K4A_ENV_VAR_LOG_TO_A_FILE[] = "K4A_ENABLE_LOG_TO_A_FILE_TEST";

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
}

int k4a_test_common_main(int argc, char **argv)
{
    k4a_unittest_init();

    ::testing::InitGoogleTest(&argc, argv);

    int ret = RUN_ALL_TESTS();

    k4a_unittest_deinit();

    return ret;
}

int64_t k4a_unittest_get_max_sync_delay_ms(k4a_fps_t fps)
{
    int64_t max_delay = 0;
    switch (fps)
    {
    case K4A_FRAMES_PER_SECOND_5:
        max_delay = 660;
        break;
    case K4A_FRAMES_PER_SECOND_15:
        max_delay = 220;
        break;
    case K4A_FRAMES_PER_SECOND_30:
        max_delay = 110;
        break;
    }
    return max_delay;
}
}
