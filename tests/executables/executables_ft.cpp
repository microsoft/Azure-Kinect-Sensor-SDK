// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Tests for executables. These tests will run binaries that are created as part of the build, store their outputs to
// files, and ensure that the contents of those files are the contents that we expect.

#include <iostream>
#include <string>
#include <regex>
#include <fstream>
#include <vector>
#include <k4a/k4a.h>
#include <utcommon.h>
#include <gtest/gtest.h>

#ifdef _WIN32
#define PATH_TO_BIN(binary) binary ".exe"
#define MKDIR(path) "if not exist " + path + " mkdir " + path
#define RMDIR(path) "rmdir /S /Q " + path
#define POPEN _popen
#define PCLOSE _pclose
#define SETENV(env, value) _putenv_s(env, value)
#else
#define PATH_TO_BIN(binary) "./" binary
#define MKDIR(path) "mkdir -p " + path
#define RMDIR(path) "rm -rf " + path
#define POPEN popen
#define PCLOSE pclose
#define SETENV(env, value) setenv(env, value, 1)
#endif

const static std::string TEST_TEMP_DIR = "executables_test_temp";

// run the specified executable and record the output to output_path
// if output_path is empty, just output to stdout
static int run_and_record_executable(std::string shell_command_path, std::string output_path)
{
    std::string formatted_command = shell_command_path;
    if (!output_path.empty())
    {
        formatted_command += " > " + output_path + " 2>&1";
    }
    // In Linux, forking a process causes the under buffers to be forked, too. So, because popen uses fork under the
    // hood, there may have been a risk of printing something in both processes. I'm not sure if this could happen in
    // this situation, but better safe than sorry.
    std::cout << "Running: " << formatted_command << std::endl;
    std::cout.flush();
    FILE *process_stream = POPEN(formatted_command.c_str(), "r");
    if (!process_stream)
    {
        std::cout << "process_stream is NULL" << std::endl;
        return EXIT_FAILURE; // if popen fails, it returns null, which is an error
    }
    int return_code = PCLOSE(process_stream);
    std::cout << "<==============================================" << std::endl;
    try
    {
        if (!output_path.empty())
        {
            std::ifstream file(output_path);
            if (file.is_open())
            {
                std::string line;
                while (getline(file, line))
                {
                    // Using cout for these logs was causing cout to get into a bad state, which would stop delivering
                    // output messages.
                    printf("%s\n", line.c_str());
                }
                file.close();
            }
        }
    }
    catch (std::exception &e)
    {
        std::cout << "Dumping log file threw a std::exception: " << e.what() << std::endl;
    }
    std::cout << "==============================================>" << std::endl;
    return return_code;
}

/*
 * This function is used to verify test output. Specifically, it takes a stream (for the test output being checked) and
 * a list of regular expressions. This function ensures that each regular expression provided matches a line in the
 * output, in order. For example:
 *
 * input_stream:
 * The device is plugged in on port 8.
 * Today is Wednesday.
 * The device is ready for use.
 *
 * Case 1
 * regexes: { "The device is plugged in on port [0-9]+.", "The device is ready for use." }
 *
 * Pass; the first regex is tested against the first line and it matches. The second regex is tested against the second
 * line and it does not match. So, the line is passed over, and the second regex is tested against the third line and
 * does match.
 *
 * Case 2
 * regexes: { "Today is Monday.", "The device is plugged .*" }
 *
 * Fail; the first regex is tested against the first line. It doesn't match, so the first regex is tested against the
 * third line. It passes, so we move on to the third line and the second regex, which don't match. Note the importance
 * of ordering.
 */
static void test_stream_against_regexes(std::istream *input_stream, const std::vector<std::string> *regexes)
{
    auto regex_iter = regexes->cbegin();
    // ensure that all regexes are matched before the end of the file, in this order
    while (*input_stream && regex_iter != regexes->cend())
    {
        std::string results;
        getline(*input_stream, results);
        std::regex desired_out(*regex_iter);
        if (std::regex_match(results, desired_out))
        {
            ++regex_iter;
        }
    }
    EXPECT_EQ(regex_iter, regexes->cend()) << "Reached the end of the executable output before matching all of the "
                                              "required regular expressions.\nExpected \""
                                           << *regex_iter << "\", but never saw it.";
}

class executables_ft : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // environment variables are only set for this process and processes it creates, so this won't mess up the
        // environment for the user in the future.
        SETENV("K4A_ENABLE_LOG_TO_STDOUT", "0");
        SETENV("K4A_LOG_LEVEL", "i");
        ASSERT_EQ(run_and_record_executable(MKDIR(TEST_TEMP_DIR), ""), EXIT_SUCCESS);
    }

    void TearDown() override
    {
        ASSERT_EQ(run_and_record_executable(RMDIR(TEST_TEMP_DIR), ""), EXIT_SUCCESS);
    }
};

#if (USE_CUSTOM_TEST_CONFIGURATION == 0)
TEST_F(executables_ft, calibration)
{
    const std::string calibration_path = PATH_TO_BIN("calibration_info");
    const std::string calibration_out = TEST_TEMP_DIR + "/calibration-out.txt";

    ASSERT_EQ(run_and_record_executable(calibration_path, calibration_out), EXIT_SUCCESS);
    std::ifstream results(calibration_out.c_str());
    std::vector<std::string> regexes{ "Found [^]* connected devices:",
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

    test_stream_against_regexes(&results, &regexes);
}

TEST_F(executables_ft, enumerate)
{
    const std::string enumerate_path = PATH_TO_BIN("enumerate_devices");
    const std::string enumerate_out = TEST_TEMP_DIR + "/enumerate-out.txt";
    ASSERT_EQ(run_and_record_executable(enumerate_path, enumerate_out), EXIT_SUCCESS);
    std::ifstream results(enumerate_out.c_str());
    std::vector<std::string> regexes{ "Found [1-5] connected devices:", "0: Device [^]*" };
    test_stream_against_regexes(&results, &regexes);
}

TEST_F(executables_ft, fastpointcloud)
{
    const std::string fastpoint_path = PATH_TO_BIN("fastpointcloud");
    const std::string fastpoint_write_file = TEST_TEMP_DIR + "/fastpointcloud-record.txt";
    const std::string fastpoint_stdout_file = TEST_TEMP_DIR + "/fastpointcloud-stdout.txt";
    ASSERT_EQ(run_and_record_executable(fastpoint_path + " " + fastpoint_write_file, fastpoint_stdout_file),
              EXIT_SUCCESS);

    std::ifstream fastpointcloud_results(fastpoint_write_file.c_str());
    ASSERT_TRUE(fastpointcloud_results.good());

    std::vector<std::string> regexes{ "ply",
                                      "format ascii 1.0",
                                      "element vertex [0-9]+",
                                      "property float x",
                                      "property float y",
                                      "property float z",
                                      "end_header" };

    test_stream_against_regexes(&fastpointcloud_results, &regexes);
}

#ifdef USE_OPENCV
TEST_F(executables_ft, opencv_compatibility)
{
    const std::string opencv_dir = TEST_TEMP_DIR;
    const std::string opencv_path = PATH_TO_BIN("opencv_example");
    const std::string opencv_out = TEST_TEMP_DIR + "/opencv-out.txt";
    ASSERT_EQ(run_and_record_executable(opencv_path, opencv_out), EXIT_SUCCESS);

    std::ifstream opencv_results(opencv_out);
    ASSERT_TRUE(opencv_results.good());

    std::vector<std::string> regexes{ "3d point:.*", "OpenCV projectPoints:.*", "k4a_calibration_3d_to_2d:.*" };

    test_stream_against_regexes(&opencv_results, &regexes);
}
#endif

TEST_F(executables_ft, streaming)
{
    const std::string streaming_path = PATH_TO_BIN("streaming_samples");
    const std::string streaming_stdout_file = TEST_TEMP_DIR + "/streaming-stdout.txt";
    ASSERT_EQ(run_and_record_executable(streaming_path + " 20", streaming_stdout_file), EXIT_SUCCESS);

    std::ifstream streaming_results(streaming_stdout_file);
    ASSERT_TRUE(streaming_results.good());

    std::vector<std::string>
        regexes{ "Capturing 20 frames",
                 "Capture \\| Color res:[0-9]+x[0-9]+ stride: [^\\|]*\\| Ir16 res: [0-9]+x [0-9]+ stride: "
                 "[0-9]+[^\\|]*\\| Depth16 res: [0-9]+x [0-9]+ stride: [0-9]+" };

    test_stream_against_regexes(&streaming_results, &regexes);
}

TEST_F(executables_ft, transformation)
{
    const std::string transformation_dir = TEST_TEMP_DIR;
    const std::string transformation_path = PATH_TO_BIN("transformation_example");
    const std::string transformation_stdout_file = TEST_TEMP_DIR + "/transformation-stdout.txt";
    ASSERT_EQ(run_and_record_executable(transformation_path + " capture " + transformation_dir + " 0",
                                        transformation_stdout_file),
              EXIT_SUCCESS);

    std::ifstream transformation_results_d2c(transformation_dir + "/depth_to_color.ply");
    ASSERT_TRUE(transformation_results_d2c.good());

    std::vector<std::string> regexes{ "ply",
                                      "format ascii 1.0",
                                      "element vertex [0-9]+",
                                      "property float x",
                                      "property float y",
                                      "property float z",
                                      "property uchar red",
                                      "property uchar green",
                                      "property uchar blue",
                                      "end_header" };

    std::ifstream transformation_results_c2d(transformation_dir + "/depth_to_color.ply");
    ASSERT_TRUE(transformation_results_c2d.good());

    test_stream_against_regexes(&transformation_results_c2d, &regexes);
    test_stream_against_regexes(&transformation_results_d2c, &regexes);
}

TEST_F(executables_ft, undistort)
{
    const std::string undistort_path = PATH_TO_BIN("undistort");
    const std::string undistort_write_file = TEST_TEMP_DIR + "/undistort-record.csv";
    ASSERT_EQ(run_and_record_executable(undistort_path + " 2 " + undistort_write_file, ""), EXIT_SUCCESS);

    std::ifstream undistort_results(undistort_write_file);
    // don't bother checking the csv file- just make sure it's there
    ASSERT_TRUE(undistort_results.good());
}
#endif // USE_CUSTOM_TEST_CONFIGURATION == 0

#if (USE_CUSTOM_TEST_CONFIGURATION == 1)
#ifdef USE_OPENCV
TEST_F(executables_ft, green_screen_single_cam)
#else
TEST_F(executables_ft, DISABLED_green_screen_single_cam)
#endif
{
    const std::string green_screen_path = PATH_TO_BIN("green_screen");
    const std::string green_screen_out = TEST_TEMP_DIR + "/green_screen-single-out.txt";
    // Calibration timeout for this is 10min due to low light conditions in the lab and slow perf of
    // cv::findChessboardCorners.
    ASSERT_EQ(run_and_record_executable(green_screen_path + " 1 9 6 22 1000 4000 2 600 5", green_screen_out),
              EXIT_SUCCESS);
}

#ifdef USE_OPENCV
TEST_F(executables_ft, green_screen_double_cam)
#else
TEST_F(executables_ft, DISABLED_green_screen_double_cam)
#endif
{
    const std::string green_screen_path = PATH_TO_BIN("green_screen");
    const std::string green_screen_out = TEST_TEMP_DIR + "/green_screen-double-out.txt";
    // Calibration timeout for this is 10min due to low light conditions in the lab and slow perf of
    // cv::findChessboardCorners.
    ASSERT_EQ(run_and_record_executable(green_screen_path + " 2 9 6 22 1000 4000 2 600 5", green_screen_out),
              EXIT_SUCCESS);
    std::ifstream results(green_screen_out.c_str());
    std::vector<std::string> regexes{ "Finished calibrating!" };
    test_stream_against_regexes(&results, &regexes);
}
#endif // USE_CUSTOM_TEST_CONFIGURATION == 1

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
