// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream> // TODO remove
#include <string>
#include <fstream>
#include <k4a/k4a.h>
#include <utcommon.h>
#include <gtest/gtest.h>

TEST(examples_ft, calibration)
{
    ASSERT_EQ(true, true);
}

TEST(examples_ft, enumerate)
{
    ASSERT_EQ(true, false);
}

TEST(examples_ft, fastpointcloud)
{
    // TODO fflush needed here?
    // TODO better place to dump
    // TODO needs to be random?
    std::string fastpoint_out = "/tmp/fastpointcloud-test-out";
    std::string fastpoint_command;
    // TODO less ugly
    fastpoint_command += "./fastpointcloud ";
    fastpoint_command += fastpoint_out;
    fastpoint_command += " 2>&1";
    FILE *first = popen(fastpoint_command.c_str(), "r");
    ASSERT_NE(first, nullptr);
    // TODO errno?
    // ASSERT_NE(fgets(buffer, 10000, first), nullptr);

    ASSERT_EQ(pclose(first), EXIT_SUCCESS);

    std::ifstream fastpointcloud_results(fastpoint_out.c_str());
    std::string s;
    while (fastpointcloud_results >> s)
    {
        std::cout << s << std::endl;
    }
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
