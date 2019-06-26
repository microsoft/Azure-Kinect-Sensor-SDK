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

const static std::string TEST_TEMP_DIR = "examples_test_temp";

// run the specified executable and record the output to output_path
// if output_path is empty, just output to stdout
static void run_and_record_executable(const std::string &executable_path,
                                      const std::string &args,
                                      const std::string &output_path)
{
    // TODO fflush needed here?
    std::string formatted_command = executable_path;
    formatted_command += " " + args;
    if (!output_path.empty())
    {
        formatted_command += " 2>&1 > " + output_path;
    }
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
}

static void test_stream_against_regexes(std::istream &input_stream, const std::vector<std::string> &regexes)
{
    auto regex_iter = regexes.cbegin();
    // ensure that all regexes are matched before the end of the file, in this order
    while (regex_iter != regexes.cend() && input_stream)
    {
        std::string results;
        getline(input_stream, results);
        std::regex desired_out(*regex_iter);
        std::cout << results << std::endl; // TODO delete this
        ++regex_iter;
    }
    ASSERT_EQ(regex_iter, regexes.cend())
        << "Reached the end of the example output before matching all of the required regular expressions.";
}

class examples_ft : public ::testing::Test
{
protected:
    void SetUp() override
    {
#ifdef _WIN32
        // _putenv_s("K4A_ENABLE_LOG_TO_STDOUT", "0");
        // _putenv_s("K4A_LOG_LEVEL", "i");
#else
        setenv("K4A_ENABLE_LOG_TO_STDOUT", "0", 1);
        setenv("K4A_LOG_LEVEL", "i", 1);
#endif

#ifdef _WIN32
        run_and_record_executable("if not exist " + TEST_TEMP_DIR + " mkdir", TEST_TEMP_DIR, "");
#else
        run_and_record_executable("mkdir -p", TEST_TEMP_DIR, "");
#endif
    }

    void TearDown() override
    {
#ifdef _WIN32
        run_and_record_executable("rmdir /S /Q", TEST_TEMP_DIR, ""); // TODO might not even need this
#else
        run_and_record_executable("rm -rf", TEST_TEMP_DIR, ""); // TODO might not even need this
#endif
    }
};

TEST_F(examples_ft, calibration)
{
#ifdef _WIN32
    const std::string calibration_path = "bin\\calibration_info.exe";
    const std::string calibration_out = TEST_TEMP_DIR + "\\calibration-out.txt";
#else
    const std::string calibration_path = "./bin/calibration_info";
    const std::string calibration_out = TEST_TEMP_DIR + "/calibration-out.txt";
#endif

    // get the calibration output
    run_and_record_executable(calibration_path, "", calibration_out);

    std::ifstream results(calibration_out.c_str());
    // make sure the calibration output has the right format
    std::vector<std::string> regexes{ "Found [^]* connected devices:",
                                      "",
                                      "===== Device [^]* =====",
                                      "resolution width: [^]*",
                                      "resolution height: [^]*",
                                      "principal point x: [^]*",
                                      "principal point y: [^]*",
                                      "focal length x: [^]*",
                                      "focal length y: [^]*",
                                      "radial distortion coefficients:",
                                      "k1: [^]*",
                                      "k2: [^]*",
                                      "k3: [^]*",
                                      "k4: [^]*",
                                      "k5: [^]*",
                                      "k6: [^]*",
                                      "center of distortion in Z=1 plane, x: [^]*",
                                      "center of distortion in Z=1 plane, y: [^]*",
                                      "tangential distortion coefficient x: [^]*",
                                      "tangential distortion coefficient y: [^]*",
                                      "metric radius: [^]*" };

    test_stream_against_regexes(results, regexes);
}

TEST_F(examples_ft, enumerate)
{
    // TODO complete
    ASSERT_EQ(true, true);
}

TEST_F(examples_ft, fastpointcloud)
{
#ifdef _WIN32
    const std::string fastpoint_write_file = TEST_TEMP_DIR + "\\fastpointcloud-record.txt";
    const std::string fastpoint_stdout_file = TEST_TEMP_DIR + "\\fastpointcloud-stdout.txt";
    const std::string fastpoint_path = "bin\\fastpointcloud.exe";
#else
    const std::string fastpoint_write_file = TEST_TEMP_DIR + "/fastpointcloud-record.txt";
    const std::string fastpoint_stdout_file = TEST_TEMP_DIR + "/fastpointcloud-stdout.txt";
    const std::string fastpoint_path = "./bin/fastpointcloud";
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

    test_stream_against_regexes(fastpointcloud_results, regexes);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
