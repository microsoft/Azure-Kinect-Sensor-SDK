#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
#include <k4a/k4a.h>
using namespace std;

static string get_serial(k4a_device_t device)
{
    size_t serial_number_length = 0;

    if (K4A_BUFFER_RESULT_TOO_SMALL != k4a_device_get_serialnum(device, NULL, &serial_number_length))
    {
        cout << "Failed to get serial number length" << endl;
        k4a_device_close(device);
        exit(-1);
    }

    char *serial_number = new (std::nothrow) char[serial_number_length];
    if (serial_number == NULL)
    {
        cout << "Failed to allocate memory for serial number (" << serial_number_length << " bytes)" << endl;
        k4a_device_close(device);
        exit(-1);
    }

    if (K4A_BUFFER_RESULT_SUCCEEDED != k4a_device_get_serialnum(device, serial_number, &serial_number_length))
    {
        cout << "Failed to get serial number" << endl;
        delete[] serial_number;
        serial_number = NULL;
        k4a_device_close(device);
        exit(-1);
    }

    string s(serial_number);
    delete[] serial_number;
    serial_number = NULL;
    return s;
}

static void print_calibration()
{
    uint32_t device_count = k4a_device_get_installed_count();
    cout << "Found " << device_count << " connected devices:" << endl;
    cout << fixed << setprecision(6);

    for (uint8_t deviceIndex = 0; deviceIndex < device_count; deviceIndex++)
    {
        k4a_device_t device = NULL;
        if (K4A_RESULT_SUCCEEDED != k4a_device_open(deviceIndex, &device))
        {
            cout << deviceIndex << ": Failed to open device" << endl;
            exit(-1);
        }

        k4a_calibration_t calibration;

        // 1. declare mode infos
        k4a_color_mode_info_t color_mode_info = { sizeof(k4a_color_mode_info_t), K4A_ABI_VERSION, 0 };
        k4a_depth_mode_info_t depth_mode_info = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, 0 };
        k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, 0 };

        // 2. initialize default mode ids
        uint32_t color_mode_id = 0;
        uint32_t depth_mode_id = 0;
        uint32_t fps_mode_id = 0;

        // 3. get the count of modes
        uint32_t color_mode_count = 0;
        uint32_t depth_mode_count = 0;
        uint32_t fps_mode_count = 0;

        if (!k4a_device_get_color_mode_count(device, &color_mode_count) == K4A_RESULT_SUCCEEDED)
        {
            cout << "Failed to get color mode count" << endl;
            exit(-1);
        }

        if (!k4a_device_get_depth_mode_count(device, &depth_mode_count) == K4A_RESULT_SUCCEEDED)
        {
            cout << "Failed to get depth mode count" << endl;
            exit(-1);
        }

        if (!k4a_device_get_fps_mode_count(device, &fps_mode_count) == K4A_RESULT_SUCCEEDED)
        {
            cout << "Failed to get fps mode count" << endl;
            exit(-1);
        }

        // 4. find the mode ids you want
        if (color_mode_count > 1)
        {
            for (uint32_t c = 1; c < color_mode_count; c++)
            {
                k4a_color_mode_info_t color_mode = { sizeof(k4a_color_mode_info_t), K4A_ABI_VERSION, 0 };
                if (k4a_device_get_color_mode(device, c, &color_mode) == K4A_RESULT_SUCCEEDED)
                {
                    if (color_mode.height >= 1080)
                    {
                        color_mode_id = c;
                        break;
                    }
                }
            }
        }

        if (depth_mode_count > 1)
        {
            for (uint32_t d = 1; d < depth_mode_count; d++)
            {
                k4a_depth_mode_info_t depth_mode = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, 0 };
                if (k4a_device_get_depth_mode(device, d, &depth_mode) == K4A_RESULT_SUCCEEDED)
                {
                    if (depth_mode.height >= 576 && depth_mode.vertical_fov <= 65)
                    {
                        depth_mode_id = d;
                        break;
                    }
                }
            }
        }

        if (fps_mode_count > 1)
        {
            uint32_t max_fps = 0;
            for (uint32_t f = 1; f < fps_mode_count; f++)
            {
                k4a_fps_mode_info_t fps_mode = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, 0 };
                if (k4a_device_get_fps_mode(device, f, &fps_mode) == K4A_RESULT_SUCCEEDED)
                {
                    if (fps_mode.fps >= (int)max_fps)
                    {
                        max_fps = (uint32_t)fps_mode.fps;
                        fps_mode_id = f;
                    }
                }
            }
        }

        // 5. fps mode id must not be set to 0, which is Off, and either color mode id or depth mode id must not be set to 0
        if (fps_mode_id == 0)
        {
            cout << "Fps mode id must not be set to 0 (Off)" << endl;
            exit(-1);
        }

        if (color_mode_id == 0 && depth_mode_id == 0)
        {
            cout << "Either color mode id or depth mode id must not be set to 0 (Off)" << endl;
            exit(-1);
        }

        // 6. use the mode ids to get the modes
        k4a_device_get_color_mode(device, color_mode_id, &color_mode_info);
        k4a_device_get_depth_mode(device, depth_mode_id, &depth_mode_info);
        k4a_device_get_fps_mode(device, fps_mode_id, &fps_mode_info);

        // get calibration
        if (K4A_RESULT_SUCCEEDED !=
            k4a_device_get_calibration(device, depth_mode_info.mode_id, color_mode_info.mode_id, &calibration))
        {
            cout << "Failed to get calibration" << endl;
            exit(-1);
        }

        auto calib = calibration.depth_camera_calibration;

        cout << "\n===== Device " << (int)deviceIndex << ": " << get_serial(device) << " =====\n";
        cout << "resolution width: " << calib.resolution_width << endl;
        cout << "resolution height: " << calib.resolution_height << endl;
        cout << "principal point x: " << calib.intrinsics.parameters.param.cx << endl;
        cout << "principal point y: " << calib.intrinsics.parameters.param.cy << endl;
        cout << "focal length x: " << calib.intrinsics.parameters.param.fx << endl;
        cout << "focal length y: " << calib.intrinsics.parameters.param.fy << endl;
        cout << "radial distortion coefficients:" << endl;
        cout << "k1: " << calib.intrinsics.parameters.param.k1 << endl;
        cout << "k2: " << calib.intrinsics.parameters.param.k2 << endl;
        cout << "k3: " << calib.intrinsics.parameters.param.k3 << endl;
        cout << "k4: " << calib.intrinsics.parameters.param.k4 << endl;
        cout << "k5: " << calib.intrinsics.parameters.param.k5 << endl;
        cout << "k6: " << calib.intrinsics.parameters.param.k6 << endl;
        cout << "center of distortion in Z=1 plane, x: " << calib.intrinsics.parameters.param.codx << endl;
        cout << "center of distortion in Z=1 plane, y: " << calib.intrinsics.parameters.param.cody << endl;
        cout << "tangential distortion coefficient x: " << calib.intrinsics.parameters.param.p1 << endl;
        cout << "tangential distortion coefficient y: " << calib.intrinsics.parameters.param.p2 << endl;
        cout << "metric radius: " << calib.intrinsics.parameters.param.metric_radius << endl;

        k4a_device_close(device);
    }
}

static void calibration_blob(uint8_t deviceIndex = 0, string filename = "calibration.json")
{
    k4a_device_t device = NULL;

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(deviceIndex, &device))
    {
        cout << deviceIndex << ": Failed to open device" << endl;
        exit(-1);
    }

    size_t calibration_size = 0;
    k4a_buffer_result_t buffer_result = k4a_device_get_raw_calibration(device, NULL, &calibration_size);
    if (buffer_result == K4A_BUFFER_RESULT_TOO_SMALL)
    {
        vector<uint8_t> calibration_buffer = vector<uint8_t>(calibration_size);
        buffer_result = k4a_device_get_raw_calibration(device, calibration_buffer.data(), &calibration_size);
        if (buffer_result == K4A_BUFFER_RESULT_SUCCEEDED)
        {
            ofstream file(filename, ofstream::binary);
            file.write(reinterpret_cast<const char *>(&calibration_buffer[0]), (long)calibration_size);
            file.close();
            cout << "Calibration blob for device " << (int)deviceIndex << " (serial no. " << get_serial(device)
                 << ") is saved to " << filename << endl;
        }
        else
        {
            cout << "Failed to get calibration blob" << endl;
            exit(-1);
        }
    }
    else
    {
        cout << "Failed to get calibration blob size" << endl;
        exit(-1);
    }
}

static void print_usage()
{
    cout << "Usage: calibration_info [device_id] [output_file]" << endl;
    cout << "Using calibration_info.exe without any command line arguments will display" << endl
         << "calibration info of all connected devices in stdout. If a device_id is given" << endl
         << "(0 for default device), the calibration.json file of that device will be" << endl
         << "saved to the current directory." << endl;
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        print_calibration();
    }
    else if (argc == 2)
    {
        calibration_blob((uint8_t)atoi(argv[1]), "calibration.json");
    }
    else if (argc == 3)
    {
        calibration_blob((uint8_t)atoi(argv[1]), argv[2]);
    }
    else
    {
        print_usage();
    }

    return 0;
}
