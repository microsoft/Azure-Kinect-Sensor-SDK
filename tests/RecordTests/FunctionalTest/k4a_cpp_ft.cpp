// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4arecord/record.hpp>
#include <k4arecord/playback.hpp>
#include <utcommon.h>

#include <gtest/gtest.h>

//#include <azure_c_shared_utility/lock.h>
//#include <azure_c_shared_utility/threadapi.h>
//#include <azure_c_shared_utility/condition.h>

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

TEST_F(k4a_cpp_ft, record) {}