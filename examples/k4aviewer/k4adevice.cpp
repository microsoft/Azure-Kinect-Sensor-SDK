/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4adevice.h"

// System headers
//
#include <sstream>

// Library headers
//

// Project headers
//
#include "k4acapture.h"

using namespace k4aviewer;

namespace
{
constexpr int32_t StartupCaptureTimeoutMs = 10000;
constexpr int MaxCaptureTimeoutCount = 120;

template<typename T> void NotifyObservers(std::list<std::weak_ptr<IK4AObserver<T>>> &observerList, T &value)
{
    for (auto wpObserver = observerList.begin(); wpObserver != observerList.end();)
    {
        auto spObserver = wpObserver->lock();
        if (spObserver)
        {
            spObserver->NotifyData(value);
            ++wpObserver;
        }
        else
        {
            auto toDelete = wpObserver;
            ++wpObserver;
            observerList.erase(toDelete);
        }
    }
}
} // namespace

K4ADevice::K4ADevice(const k4a_device_t device) : m_device(device)
{
    // Load firmware version
    //
    const k4a_result_t versionResult = k4a_device_get_version(device, &m_firmwareVersion);
    if (versionResult != K4A_RESULT_SUCCEEDED)
    {
        memset(&m_firmwareVersion, 0, sizeof(m_firmwareVersion));
    }

    // Load serial number
    //
    size_t serialNumberBufferSize = 0;
    k4a_buffer_result_t getSerialNumberResult = k4a_device_get_serialnum(device,
                                                                         &m_serialNumber[0],
                                                                         &serialNumberBufferSize);

    // On some prototype devices, we get back an empty serial number (just the '\0').
    // If that happens, we want to report 'unknown device' rather than not printing an identifier.
    //
    if (getSerialNumberResult == K4A_BUFFER_RESULT_TOO_SMALL && serialNumberBufferSize > 1)
    {
        m_serialNumber.resize(serialNumberBufferSize);
        getSerialNumberResult = k4a_device_get_serialnum(device, &m_serialNumber[0], &serialNumberBufferSize);
        if (getSerialNumberResult == K4A_BUFFER_RESULT_SUCCEEDED)
        {
            // std::string expects there to not be as null terminator at the end of its data but
            // k4a_device_get_serialnum adds a null terminator, so we drop the last character of the string after we get
            // the result back.
            //
            m_serialNumber.resize(serialNumberBufferSize - 1);
        }
    }

    if (getSerialNumberResult != K4A_BUFFER_RESULT_SUCCEEDED)
    {
        static int unknownDeviceId = 0;
        unknownDeviceId++;
        std::stringstream nameBuilder;
        nameBuilder << "[Unknown K4A device #" << unknownDeviceId << "]";
        m_serialNumber = nameBuilder.str();
    }
}

K4ADevice::~K4ADevice()
{
    if (m_device)
    {
        StopCameras();
        StopImu();
        k4a_device_close(m_device);
    }
}

k4a_result_t K4ADevice::StartCameras(k4a_device_configuration_t &configuration)
{
    const k4a_result_t result = k4a_device_start_cameras(m_device, &configuration);
    if (K4A_RESULT_SUCCEEDED != result)
    {
        return result;
    }

    m_camerasStarted = true;

    int32_t timeout = StartupCaptureTimeoutMs;

    // If we're starting in subordinate mode, we have to wait for the master to send the 'start' signal
    //
    if (configuration.wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE)
    {
        timeout = K4A_WAIT_INFINITE;
    }

    const k4a_wait_result_t waitResult = PollCameras(timeout);
    if (waitResult != K4A_WAIT_RESULT_SUCCEEDED)
    {
        StopCameras();
        return K4A_RESULT_FAILED;
    }

    m_configuration = configuration;

    return result;
}

k4a_result_t K4ADevice::StartImu()
{
    const k4a_result_t result = k4a_device_start_imu(m_device);
    if (K4A_RESULT_SUCCEEDED != result)
    {
        return result;
    }

    const k4a_wait_result_t waitResult = PollImu(StartupCaptureTimeoutMs);
    if (waitResult != K4A_WAIT_RESULT_SUCCEEDED)
    {
        return K4A_RESULT_FAILED;
    }

    m_imuStarted = true;
    return result;
}

void K4ADevice::StopCameras()
{
    k4a_device_stop_cameras(m_device);
    m_camerasStarted = false;

    for (const auto &captureObserver : m_captureObservers)
    {
        if (auto spCaptureObserver = captureObserver.lock())
        {
            spCaptureObserver->NotifyTermination();
        }
    }
}

void K4ADevice::StopImu()
{
    k4a_device_stop_imu(m_device);
    m_imuStarted = false;
}

k4a_wait_result_t K4ADevice::PollCameras(const int32_t timeoutMs)
{
    if (!m_camerasStarted)
    {
        return K4A_WAIT_RESULT_FAILED;
    }

    k4a_capture_t newCapture;

    const k4a_wait_result_t waitResult = k4a_device_get_capture(m_device, &newCapture, timeoutMs);

    if (K4A_WAIT_RESULT_SUCCEEDED != waitResult)
    {
        return waitResult;
    }

    auto spCapture = std::make_shared<K4ACapture>(newCapture);

    NotifyObservers(m_captureObservers, spCapture);

    return waitResult;
}

k4a_wait_result_t K4ADevice::PollImu(const int32_t timeoutMs)
{
    k4a_imu_sample_t sample;

    const k4a_wait_result_t result = k4a_device_get_imu_sample(m_device, &sample, timeoutMs);
    if (result != K4A_WAIT_RESULT_SUCCEEDED)
    {
        return result;
    }

    NotifyObservers(m_imuObservers, sample);

    return result;
}

k4a_result_t K4ADevice::GetSyncCablesConnected(bool &syncInCableConnected, bool &syncOutCableConnected)
{
    const k4a_result_t jackResult = k4a_device_get_sync_jack(m_device, &syncInCableConnected, &syncOutCableConnected);
    if (jackResult != K4A_RESULT_SUCCEEDED)
    {
        syncInCableConnected = false;
        syncOutCableConnected = false;
    }

    return jackResult;
}

k4a_result_t K4ADevice::GetCalibrationTransformData(std::unique_ptr<K4ACalibrationTransformData> &calibrationData,
                                                    const k4a_depth_mode_t depthMode,
                                                    const k4a_color_resolution_t colorResolution) const
{
    calibrationData.reset(new K4ACalibrationTransformData);
    return calibrationData->Initialize(m_device, depthMode, colorResolution);
}

k4a_result_t K4ADevice::SetColorControl(k4a_color_control_command_t command,
                                        k4a_color_control_mode_t targetMode,
                                        int32_t value)
{
    return k4a_device_set_color_control(m_device, command, targetMode, value);
}

k4a_result_t K4ADevice::GetColorControl(k4a_color_control_command_t command,
                                        k4a_color_control_mode_t &targetMode,
                                        int32_t &value)
{
    return k4a_device_get_color_control(m_device, command, &targetMode, &value);
}

void K4ADevice::RegisterCaptureObserver(std::shared_ptr<IK4ACaptureObserver> &&captureObserver)
{
    m_captureObservers.emplace_back(std::move(captureObserver));
}

void K4ADevice::RegisterImuObserver(std::shared_ptr<IK4AImuObserver> &&imuObserver)
{
    m_imuObservers.emplace_back(std::move(imuObserver));
}

k4a_wait_result_t K4ADevice::PollCameras()
{
    const k4a_wait_result_t result = CheckTimeout(PollCameras(0), &m_cameraTimeoutCounter);

    if (result == K4A_WAIT_RESULT_FAILED)
    {
        StopCameras();
    }

    return result;
}

k4a_wait_result_t K4ADevice::PollImu()
{
    const k4a_wait_result_t result = CheckTimeout(PollImu(0), &m_imuTimeoutCounter);

    if (result == K4A_WAIT_RESULT_FAILED)
    {
        StopImu();
    }

    return result;
}

k4a_wait_result_t K4ADevice::CheckTimeout(const k4a_wait_result_t result, int *timeoutCounter)
{
    if (result == K4A_WAIT_RESULT_SUCCEEDED)
    {
        *timeoutCounter = 0;
        return result;
    }

    if (result == K4A_WAIT_RESULT_TIMEOUT)
    {
        if (*timeoutCounter < MaxCaptureTimeoutCount)
        {
            ++*timeoutCounter;
            return K4A_WAIT_RESULT_SUCCEEDED;
        }

        return K4A_WAIT_RESULT_FAILED;
    }

    return result;
}

k4a_result_t K4ADeviceFactory::OpenDevice(std::shared_ptr<K4ADevice> &device, const uint8_t sensorId)
{
    k4a_device_t sensor;
    if (K4A_RESULT_SUCCEEDED != k4a_device_open(sensorId, &sensor))
    {
        return K4A_RESULT_FAILED;
    }

    device.reset(new K4ADevice(sensor));

    return K4A_RESULT_SUCCEEDED;
}
