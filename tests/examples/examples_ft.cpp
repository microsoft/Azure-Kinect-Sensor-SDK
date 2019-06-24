// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream> // TODO remove
#include <string>
#include <regex>
#include <fstream>
#include <vector>
#include <k4a/k4a.h>
#include <utcommon.h>
#include <gtest/gtest.h>

static void run_and_record_executable(const std::string &executable_path,
                                      const std::string &args,
                                      const std::string &output_path)
{
    // TODO fflush needed here?
    std::string formatted_command = executable_path;
    formatted_command += " " + args + " 2>&1 > " + output_path;
    std::cout << formatted_command << std::endl;
#ifdef _WIN32
    FILE *process_stream = _popen(formatted_command.c_str(), "r");
#else
    FILE *process_stream = popen(formatted_command.c_str(), "r");
#endif
    ASSERT_TRUE(process_stream);

    // TODO errno?
    // ASSERT_NE(fgets(buffer, 10000, first), nullptr);
    // TODO temporary, make better
    // Not in hereASSERT_NE(process_stream, nullptr);, EXIT_SUCCESS
#ifdef _WIN32
    ASSERT_EQ(_pclose(process_stream), EXIT_SUCCESS);
#else
    ASSERT_EQ(pclose(process_stream), EXIT_SUCCESS);
#endif
    // throw error maybe or something?
}

TEST(examples_ft, calibration)
{
    // TODO complete
    ASSERT_EQ(true, true);
}

TEST(examples_ft, enumerate)
{
    // TODO complete
    ASSERT_EQ(true, false);
}

TEST(examples_ft, fastpointcloud)
{
#ifdef _WIN32
    std::string fastpoint_write_file = "examples_test_temp\\fastpointcloud-test-record";
    std::string fastpoint_stdout_file = "examples_test_temp\\fastpointcloud-test-stdout";
    std::string fastpoint_path = "bin\\fastpointcloud.exe";
#else
    std::string fastpoint_write_file = "examples_test_temp/fastpointcloud-test-record";
    std::string fastpoint_stdout_file = "examples_test_temp/fastpointcloud-test-stdout";
    std::string fastpoint_path = "./bin/fastpointcloud";
#endif
    run_and_record_executable(fastpoint_path, fastpoint_write_file, fastpoint_stdout_file);

    std::ifstream fastpointcloud_results(fastpoint_write_file.c_str());
    ASSERT_TRUE(fastpointcloud_results.good());

    std::vector<std::string> regexes{ "ply",
                                      "format ascii 1.0",
                                      "element vertex [0-9]+",
                                      "property float x",
                                      "property float y",
                                      "property float z",
                                      "end_header" };

    for (const std::string &regex_str : regexes)
    {
        std::string results;
        getline(fastpointcloud_results, results);
        std::regex pointcloud_correctness(regex_str);
        ASSERT_TRUE(std::regex_match(results, pointcloud_correctness));
    }
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
