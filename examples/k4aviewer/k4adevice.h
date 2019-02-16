/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ADEVICE_H
#define K4ADEVICE_H

// System headers
//
#include <list>
#include <memory>
#include <string>
#include <vector>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include "ik4aobserver.h"
#include "k4acalibrationtransformdata.h"

namespace k4aviewer
{

class K4ADevice : public std::enable_shared_from_this<K4ADevice>
{
public:
    ~K4ADevice();

    k4a_result_t StartCameras(k4a_device_configuration_t &configuration);
    k4a_result_t StartImu();
    void StopCameras();
    void StopImu();

    const std::string &GetSerialNumber() const
    {
        return m_serialNumber;
    }

    const k4a_hardware_version_t &GetVersionInfo() const
    {
        return m_firmwareVersion;
    }

    const k4a_device_configuration_t &GetDeviceConfiguration() const
    {
        return m_configuration;
    }

    k4a_result_t GetSyncCablesConnected(bool &syncInCableConnected, bool &syncOutCableConnected);

    k4a_result_t GetCalibrationTransformData(std::unique_ptr<K4ACalibrationTransformData> &calibrationData,
                                             k4a_depth_mode_t depthMode,
                                             k4a_color_resolution_t colorResolution) const;

    bool CamerasAreStarted() const
    {
        return m_camerasStarted;
    }

    bool ImuIsStarted() const
    {
        return m_imuStarted;
    }

    k4a_result_t GetColorControl(k4a_color_control_command_t command,
                                 k4a_color_control_mode_t &targetMode,
                                 int32_t &value);
    k4a_result_t SetColorControl(k4a_color_control_command_t command,
                                 k4a_color_control_mode_t targetMode,
                                 int32_t value);

    k4a_wait_result_t PollCameras(std::unique_ptr<K4ACapture> &capture);
    k4a_wait_result_t PollImu(k4a_imu_sample_t &sample);

    k4a_wait_result_t PollCameras(int32_t timeoutMs, std::unique_ptr<K4ACapture> &capture);
    k4a_wait_result_t PollImu(int32_t timeoutMs, k4a_imu_sample_t &sample);

    // Devices are not copyable.
    //
    K4ADevice(const K4ADevice &) = delete;
    K4ADevice(const K4ADevice &&) = delete;
    K4ADevice &operator=(const K4ADevice &) = delete;
    K4ADevice &operator=(const K4ADevice &&) = delete;

private:
    friend class K4ADeviceFactory;

    explicit K4ADevice(k4a_device_t device);

    k4a_device_t m_device;
    k4a_device_configuration_t m_configuration{};

    std::string m_serialNumber = "";
    k4a_hardware_version_t m_firmwareVersion{};

    bool m_camerasStarted = false;
    bool m_imuStarted = false;
};

class K4ADeviceFactory
{
public:
    static k4a_result_t OpenDevice(std::shared_ptr<K4ADevice> &device, uint8_t sensorId);
};
} // namespace k4aviewer

#endif
