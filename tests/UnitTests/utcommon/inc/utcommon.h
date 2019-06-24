// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef UTCOMMON_H
#define UTCOMMON_H

#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include <k4a/k4atypes.h>
#include <ostream>

// Define the output operator for k4a_result_t types for clean test output
std::ostream &operator<<(std::ostream &s, const k4a_result_t &val);
std::ostream &operator<<(std::ostream &s, const k4a_wait_result_t &val);
std::ostream &operator<<(std::ostream &s, const k4a_buffer_result_t &val);

extern "C" {

// Initialize default k4a specific unittest behavior
void k4a_unittest_init();
void k4a_unittest_deinit();

#ifdef _WIN32
void k4a_unittest_init_logging_with_processid();
#endif

int k4a_test_common_main(int argc, char **argv);
}

#endif
