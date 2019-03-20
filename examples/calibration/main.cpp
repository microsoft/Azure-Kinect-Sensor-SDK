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

        k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        deviceConfig.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_1080P;
        deviceConfig.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
        deviceConfig.camera_fps = K4A_FRAMES_PER_SECOND_30;
        deviceConfig.wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
        deviceConfig.synchronized_images_only = true;

        // get calibration
        if (K4A_RESULT_SUCCEEDED !=
            k4a_device_get_calibration(device, deviceConfig.depth_mode, deviceConfig.color_resolution, &calibration))
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
