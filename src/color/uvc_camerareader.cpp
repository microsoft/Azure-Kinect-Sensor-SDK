#include "uvc_camerareader.h"
#include "ksmetadata.h"
#include <k4ainternal/common.h>
#include <k4ainternal/capture.h>

#define COLOR_CAMERA_VID 0x045e
#define COLOR_CAMERA_PID 0x097d // K4A

#define UVC_AUTO_EXPOSURE_MODE_MANUAL 1            // manual exposure time, manual iris
#define UVC_AUTO_EXPOSURE_MODE_AUTO 2              // auto exposure time,
#define UVC_AUTO_EXPOSURE_MODE_SHUTTER_PRIORITY 4  // manual exposure time, auto iris
#define UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY 8 // auto exposure time, manual iris

#define CONV_100USEC_TO_USEC (100)

// libUVC frame callback
static void UVCFrameCallback(uvc_frame_t *frame, void *ptr)
{
    if (ptr && frame)
    {
        UVCCameraReader *pThis = (UVCCameraReader *)ptr;
        pThis->Callback(frame);
    }
}

UVCCameraReader::UVCCameraReader() {}

UVCCameraReader::~UVCCameraReader()
{
    Shutdown();
}

k4a_result_t UVCCameraReader::Init(const char *serialNumber)
{
    if (IsInitialized())
    {
        LOG_ERROR("Camera reader is already initialized", 0);
        return K4A_RESULT_FAILED;
    }

    Shutdown(); // Make sure it's not initialized

    // Initialize libUVC
    uvc_error_t res = uvc_init(&m_pContext, NULL);
    if (res < 0)
    {
        LOG_ERROR("Failed to initialize libuvc: %s", uvc_strerror(res));
        return K4A_RESULT_FAILED;
    }

    // Find K4A Color Camera
    res = uvc_find_device(m_pContext, &m_pDevice, COLOR_CAMERA_VID, COLOR_CAMERA_PID, serialNumber);
    if (res < 0)
    {
        LOG_ERROR("Can't find UVC device: %s", uvc_strerror(res));
        Shutdown();

        return K4A_RESULT_FAILED;
    }

    // Open K4A Color Camera
    res = uvc_open(m_pDevice, &m_pDeviceHandle);
    if (res < 0)
    {
        LOG_ERROR("Can't open UVC device: %s", uvc_strerror(res));
        Shutdown();

        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t UVCCameraReader::Start(const uint32_t width,
                                    const uint32_t height,
                                    const float fps,
                                    const k4a_image_format_t imageFormat,
                                    color_cb_stream_t *pCallback,
                                    void *pCallbackContext)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, pCallback == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, pCallbackContext == NULL);
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!IsInitialized())
    {
        LOG_ERROR("Camera reader is not initialized", 0);
        return K4A_RESULT_FAILED;
    }

    if (m_streaming)
    {
        LOG_ERROR("Camera stream already started", 0);
        return K4A_RESULT_FAILED;
    }

    uvc_stream_ctrl_t ctrl;
    uvc_frame_format frameFormat = UVC_FRAME_FORMAT_UNKNOWN;
    switch (imageFormat)
    {
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
        m_output_image_format = imageFormat;
        m_input_image_format = imageFormat;

        frameFormat = UVC_COLOR_FORMAT_MJPEG;
        break;
    case K4A_IMAGE_FORMAT_COLOR_NV12:
        m_output_image_format = imageFormat;
        m_input_image_format = imageFormat;

        frameFormat = UVC_COLOR_FORMAT_NV12;
        break;
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
        m_output_image_format = imageFormat;
        m_input_image_format = imageFormat;

        frameFormat = UVC_COLOR_FORMAT_YUYV;
        break;
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        m_output_image_format = imageFormat;
        m_input_image_format = K4A_IMAGE_FORMAT_COLOR_MJPG;

        if (m_decoder == nullptr)
        {
            m_decoder = tjInitDecompress();
            if (m_decoder == nullptr)
            {
                LOG_ERROR("MJPEG decoder initialization failed\n", 0);
                return K4A_RESULT_FAILED;
            }
        }

        frameFormat = UVC_COLOR_FORMAT_MJPEG;
        break;
    default:
        LOG_ERROR("Unsupported format %d", imageFormat);
        return K4A_RESULT_FAILED;
    }

    m_width_pixels = width;
    m_height_pixels = height;

    // Set frame format
    uvc_error_t res =
        uvc_get_stream_ctrl_format_size(m_pDeviceHandle, &ctrl, frameFormat, (int)width, (int)height, (int)fps);

    if (res < 0)
    {
        LOG_ERROR("Failed to get stream control for resolution (%d, %d) - %d fps - format (%d): %s",
                  width,
                  height,
                  (int)fps,
                  imageFormat,
                  uvc_strerror(res));
        return K4A_RESULT_FAILED;
    }

    // Set callback
    m_pCallback = pCallback;
    m_pCallbackContext = pCallbackContext;

    res = uvc_start_streaming(m_pDeviceHandle, &ctrl, UVCFrameCallback, this, 0);
    if (res < 0)
    {
        LOG_ERROR("Failed to start streaming: %s", uvc_strerror(res));

        // Clear
        m_width_pixels = 0;
        m_height_pixels = 0;
        m_pCallback = nullptr;
        m_pCallbackContext = nullptr;

        return K4A_RESULT_FAILED;
    }
    m_streaming = true;

    return K4A_RESULT_SUCCEEDED;
}

void UVCCameraReader::Stop()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_streaming)
    {
        if (!IsInitialized())
        {
            LOG_WARNING("Camera reader is not initialized but in streaming state", 0);
        }

        m_streaming = false;
        m_pCallback = nullptr;
        m_pCallbackContext = nullptr;

        // Call uvc_stop_streaming() without lock.
        // uvc_stop_streaming() returns when all callbacks are completed or cancelled.
        // Calling it with lock may cause deadlock.
        lock.unlock();
        uvc_stop_streaming(m_pDeviceHandle);
    }
}

void UVCCameraReader::Shutdown()
{
    // Make sure stream is stopped
    Stop();

    if (m_pDeviceHandle)
    {
        // Close UVC device handle
        uvc_close(m_pDeviceHandle);
        m_pDeviceHandle = nullptr;
    }

    if (m_pDevice)
    {
        // Release UVC device descriptor
        uvc_unref_device(m_pDevice);
        m_pDevice = nullptr;
    }

    if (m_pContext)
    {
        // Close the UVC context
        uvc_exit(m_pContext);
        m_pContext = nullptr;
    }

    if (m_decoder)
    {
        // Destroy MJPEG decoder
        (void)tjDestroy(m_decoder);
        m_decoder = nullptr;
    }
}

k4a_result_t UVCCameraReader::GetCameraControlCapabilities(const k4a_color_control_command_t command,
                                                           color_control_cap_t *capabilities)
{
    uvc_error_t res = UVC_SUCCESS;
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, capabilities == NULL);

    switch (command)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE:
    {
        uint8_t default_ae_mode;
        uint32_t min_exposure_time;
        uint32_t max_exposure_time;
        uint32_t step_exposure_time;
        uint32_t default_exposure_time;

        res = uvc_get_ae_mode(m_pDeviceHandle, &default_ae_mode, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default auto exposure mode: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        if (default_ae_mode == UVC_AUTO_EXPOSURE_MODE_MANUAL ||
            default_ae_mode == UVC_AUTO_EXPOSURE_MODE_SHUTTER_PRIORITY)
        {
            capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        }
        else if (default_ae_mode == UVC_AUTO_EXPOSURE_MODE_AUTO ||
                 default_ae_mode == UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY)
        {
            capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_AUTO;
        }
        else
        {
            LOG_ERROR("Invalid auto exposure mode returned: %d", default_ae_mode);
            return K4A_RESULT_FAILED;
        }

        // Get range of exposure time value
        res = uvc_get_exposure_abs(m_pDeviceHandle, &min_exposure_time, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min exposure time abs: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_exposure_abs(m_pDeviceHandle, &max_exposure_time, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max exposure time abs: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_exposure_abs(m_pDeviceHandle, &step_exposure_time, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get exposure time abs: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_exposure_abs(m_pDeviceHandle, &default_exposure_time, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default exposure time abs: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        // 0.0001 sec to uSec
        capabilities->supportAuto = true;
        capabilities->stepValue = (int32_t)(step_exposure_time * CONV_100USEC_TO_USEC);
        capabilities->minValue = MapLinuxExposureToK4a((int32_t)min_exposure_time);
        capabilities->maxValue = MapLinuxExposureToK4a((int32_t)max_exposure_time);
        capabilities->defaultValue = MapLinuxExposureToK4a((int32_t)default_exposure_time);
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_BRIGHTNESS:
    {
        int16_t min_brightness;
        int16_t max_brightness;
        int16_t step_brightness;
        int16_t default_brightness;
        res = uvc_get_brightness(m_pDeviceHandle, &min_brightness, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min brightness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_brightness(m_pDeviceHandle, &max_brightness, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max brightness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_brightness(m_pDeviceHandle, &step_brightness, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get step brightness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_brightness(m_pDeviceHandle, &default_brightness, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default brightness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        capabilities->supportAuto = false;
        capabilities->minValue = (int32_t)min_brightness;
        capabilities->maxValue = (int32_t)max_brightness;
        capabilities->stepValue = (int32_t)step_brightness;
        capabilities->defaultValue = (int32_t)default_brightness;
        capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_CONTRAST:
    {
        uint16_t min_contrast;
        uint16_t max_contrast;
        uint16_t step_contrast;
        uint16_t default_contrast;

        res = uvc_get_contrast(m_pDeviceHandle, &min_contrast, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min contrast: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_contrast(m_pDeviceHandle, &max_contrast, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max contrast: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_contrast(m_pDeviceHandle, &step_contrast, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get step contrast: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_contrast(m_pDeviceHandle, &default_contrast, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default contrast: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        capabilities->supportAuto = false;
        capabilities->minValue = (int32_t)min_contrast;
        capabilities->maxValue = (int32_t)max_contrast;
        capabilities->stepValue = (int32_t)step_contrast;
        capabilities->defaultValue = (int32_t)default_contrast;
        capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_SATURATION:
    {
        uint16_t min_saturation;
        uint16_t max_saturation;
        uint16_t step_saturation;
        uint16_t default_saturation;

        res = uvc_get_saturation(m_pDeviceHandle, &min_saturation, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min saturation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_saturation(m_pDeviceHandle, &max_saturation, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max saturation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_saturation(m_pDeviceHandle, &step_saturation, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get step saturation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_saturation(m_pDeviceHandle, &default_saturation, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default saturation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        capabilities->supportAuto = false;
        capabilities->minValue = (int32_t)min_saturation;
        capabilities->maxValue = (int32_t)max_saturation;
        capabilities->stepValue = (int32_t)step_saturation;
        capabilities->defaultValue = (int32_t)default_saturation;
        capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_SHARPNESS:
    {
        uint16_t min_sharpness;
        uint16_t max_sharpness;
        uint16_t step_sharpness;
        uint16_t default_sharpness;

        res = uvc_get_sharpness(m_pDeviceHandle, &min_sharpness, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min sharpness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_sharpness(m_pDeviceHandle, &max_sharpness, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max sharpness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_sharpness(m_pDeviceHandle, &step_sharpness, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get step sharpness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_sharpness(m_pDeviceHandle, &default_sharpness, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default sharpness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        capabilities->supportAuto = false;
        capabilities->minValue = (int32_t)min_sharpness;
        capabilities->maxValue = (int32_t)max_sharpness;
        capabilities->stepValue = (int32_t)step_sharpness;
        capabilities->defaultValue = (int32_t)default_sharpness;
        capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_WHITEBALANCE:
    {
        uint8_t default_wb_mode;
        uint16_t min_white_balance;
        uint16_t max_white_balance;
        uint16_t step_white_balance;
        uint16_t default_white_balance;

        res = uvc_get_white_balance_temperature_auto(m_pDeviceHandle, &default_wb_mode, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default auto white balance temperature mode: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        if (default_wb_mode == 0)
        {
            capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        }
        else if (default_wb_mode == 1)
        {
            capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_AUTO;
        }
        else
        {
            LOG_ERROR("Invalid default auto white balance temperature mode returned: %d", default_wb_mode);
            return K4A_RESULT_FAILED;
        }

        // Get range of white balance temperature value
        res = uvc_get_white_balance_temperature(m_pDeviceHandle, &min_white_balance, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min white balance temperature: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_white_balance_temperature(m_pDeviceHandle, &max_white_balance, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max white balance temperature: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_white_balance_temperature(m_pDeviceHandle, &step_white_balance, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get step white balance temperature: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_white_balance_temperature(m_pDeviceHandle, &default_white_balance, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default white balance temperature: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        capabilities->supportAuto = true;
        capabilities->minValue = min_white_balance;
        capabilities->maxValue = max_white_balance;
        capabilities->stepValue = step_white_balance;
        capabilities->defaultValue = default_white_balance;
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION:
    {
        uint16_t min_backlight_compensation;
        uint16_t max_backlight_compensation;
        uint16_t step_backlight_compensation;
        uint16_t default_backlight_compensation;

        res = uvc_get_backlight_compensation(m_pDeviceHandle, &min_backlight_compensation, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min backlight compensation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_backlight_compensation(m_pDeviceHandle, &max_backlight_compensation, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max backlight compensation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_backlight_compensation(m_pDeviceHandle, &step_backlight_compensation, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get step backlight compensation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_backlight_compensation(m_pDeviceHandle, &default_backlight_compensation, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default backlight compensation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        capabilities->supportAuto = false;
        capabilities->minValue = (int32_t)min_backlight_compensation;
        capabilities->maxValue = (int32_t)max_backlight_compensation;
        capabilities->stepValue = (int32_t)step_backlight_compensation;
        capabilities->defaultValue = (int32_t)default_backlight_compensation;
        capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_GAIN:
    {
        uint16_t min_gain;
        uint16_t max_gain;
        uint16_t step_gain;
        uint16_t default_gain;

        res = uvc_get_gain(m_pDeviceHandle, &min_gain, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min gain: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_gain(m_pDeviceHandle, &max_gain, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max gain: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_gain(m_pDeviceHandle, &step_gain, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get step gain: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_gain(m_pDeviceHandle, &default_gain, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default gain: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        capabilities->supportAuto = false;
        capabilities->minValue = (int32_t)min_gain;
        capabilities->maxValue = (int32_t)max_gain;
        capabilities->stepValue = (int32_t)step_gain;
        capabilities->defaultValue = (int32_t)default_gain;
        capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY:
    {
        uint8_t min_powerline_freq;
        uint8_t max_powerline_freq;
        uint8_t step_powerline_freq;
        uint8_t default_powerline_freq;

        res = uvc_get_power_line_frequency(m_pDeviceHandle, &min_powerline_freq, UVC_GET_MIN);
        if (res < 0)
        {
            LOG_ERROR("Failed to get min powerline frequency: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_power_line_frequency(m_pDeviceHandle, &max_powerline_freq, UVC_GET_MAX);
        if (res < 0)
        {
            LOG_ERROR("Failed to get max powerline frequency: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_power_line_frequency(m_pDeviceHandle, &step_powerline_freq, UVC_GET_RES);
        if (res < 0)
        {
            LOG_ERROR("Failed to get step powerline frequency: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        res = uvc_get_power_line_frequency(m_pDeviceHandle, &default_powerline_freq, UVC_GET_DEF);
        if (res < 0)
        {
            LOG_ERROR("Failed to get default powerline frequency: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        capabilities->supportAuto = false;
        capabilities->minValue = (int32_t)min_powerline_freq;
        capabilities->maxValue = (int32_t)max_powerline_freq;
        capabilities->stepValue = (int32_t)step_powerline_freq;
        capabilities->defaultValue = (int32_t)default_powerline_freq;
        capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        capabilities->valid = true;
    }
    break;
    case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY:
    {
        // Deprecated control. Return 0 for all capabilities which is current behaviour.
        capabilities->supportAuto = false;
        capabilities->minValue = 0;
        capabilities->maxValue = 0;
        capabilities->stepValue = 0;
        capabilities->defaultValue = 0;
        capabilities->defaultMode = K4A_COLOR_CONTROL_MODE_MANUAL;
        capabilities->valid = true;
        LOG_WARNING("K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY is deprecated and does nothing.", 0);
    }
    break;
    default:
        LOG_ERROR("Unsupported control: %d\n", command);
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t UVCCameraReader::GetCameraControl(const k4a_color_control_command_t command,
                                               k4a_color_control_mode_t *mode,
                                               int32_t *pValue)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, mode == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, pValue == NULL);
    if (!IsInitialized())
    {
        LOG_ERROR("Camera reader is not initialized", 0);
        return K4A_RESULT_FAILED;
    }
    uvc_error_t res = UVC_SUCCESS;
    *mode = K4A_COLOR_CONTROL_MODE_MANUAL;

    switch (command)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE:
    {
        uint8_t ae_mode;
        uint32_t exposure_time;

        res = uvc_get_ae_mode(m_pDeviceHandle, &ae_mode, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get auto exposure mode: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        if (ae_mode == UVC_AUTO_EXPOSURE_MODE_MANUAL || ae_mode == UVC_AUTO_EXPOSURE_MODE_SHUTTER_PRIORITY)
        {
            *mode = K4A_COLOR_CONTROL_MODE_MANUAL;
        }
        else if (ae_mode == UVC_AUTO_EXPOSURE_MODE_AUTO || ae_mode == UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY)
        {
            *mode = K4A_COLOR_CONTROL_MODE_AUTO;
        }
        else
        {
            LOG_ERROR("Invalid auto exposure mode returned: %d", ae_mode);
            return K4A_RESULT_FAILED;
        }

        // Get current exposure time value
        res = uvc_get_exposure_abs(m_pDeviceHandle, &exposure_time, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get exposure time abs: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        *pValue = MapLinuxExposureToK4a((int32_t)exposure_time);
    }
    break;
    case K4A_COLOR_CONTROL_BRIGHTNESS:
    {
        int16_t brightness;
        res = uvc_get_brightness(m_pDeviceHandle, &brightness, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get brightness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }
        *pValue = (int32_t)brightness;
    }
    break;
    case K4A_COLOR_CONTROL_CONTRAST:
    {
        uint16_t contrast;
        res = uvc_get_contrast(m_pDeviceHandle, &contrast, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get contrast: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }
        *pValue = (int32_t)contrast;
    }
    break;
    case K4A_COLOR_CONTROL_SATURATION:
    {
        uint16_t saturation;
        res = uvc_get_saturation(m_pDeviceHandle, &saturation, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get saturation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }
        *pValue = (int32_t)saturation;
    }
    break;
    case K4A_COLOR_CONTROL_SHARPNESS:
    {
        uint16_t sharpness;
        res = uvc_get_sharpness(m_pDeviceHandle, &sharpness, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get sharpness: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }
        *pValue = (int32_t)sharpness;
    }
    break;
    case K4A_COLOR_CONTROL_WHITEBALANCE:
    {
        uint8_t wb_mode;
        uint16_t white_balance;

        res = uvc_get_white_balance_temperature_auto(m_pDeviceHandle, &wb_mode, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get auto white balance temperature mode: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        if (wb_mode == 0)
        {
            *mode = K4A_COLOR_CONTROL_MODE_MANUAL;
        }
        else if (wb_mode == 1)
        {
            *mode = K4A_COLOR_CONTROL_MODE_AUTO;
        }
        else
        {
            LOG_ERROR("Invalid auto white balance temperature mode returned: %d", wb_mode);
            return K4A_RESULT_FAILED;
        }

        // Get current white balance temperature value
        res = uvc_get_white_balance_temperature(m_pDeviceHandle, &white_balance, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get white balance temperature: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }

        *pValue = white_balance;
    }
    break;
    case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION:
    {
        uint16_t backlight_compensation;
        res = uvc_get_backlight_compensation(m_pDeviceHandle, &backlight_compensation, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get backlight compensation: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }
        *pValue = (int32_t)backlight_compensation;
    }
    break;
    case K4A_COLOR_CONTROL_GAIN:
    {
        uint16_t gain;
        res = uvc_get_gain(m_pDeviceHandle, &gain, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get gain: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }
        *pValue = (int32_t)gain;
    }
    break;
    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY:
    {
        uint8_t powerline_freq;
        res = uvc_get_power_line_frequency(m_pDeviceHandle, &powerline_freq, UVC_GET_CUR);
        if (res < 0)
        {
            LOG_ERROR("Failed to get powerline frequency: %s", uvc_strerror(res));
            return K4A_RESULT_FAILED;
        }
        *pValue = (int32_t)powerline_freq;
    }
    break;
    case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY:
    {
        *pValue = 0;
        LOG_WARNING("K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY is deprecated and does nothing.", 0);
    }
    break;
    default:
        LOG_ERROR("Unsupported control: %d\n", command);
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t UVCCameraReader::SetCameraControl(const k4a_color_control_command_t command,
                                               const k4a_color_control_mode_t mode,
                                               int32_t newValue)
{
    if (!IsInitialized())
    {
        LOG_ERROR("Camera reader is not initialized", 0);
        return K4A_RESULT_FAILED;
    }
    uvc_error_t res = UVC_SUCCESS;

    switch (command)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            res = uvc_set_ae_mode(m_pDeviceHandle, UVC_AUTO_EXPOSURE_MODE_MANUAL);
            if (res < 0)
            {
                LOG_ERROR("Failed to set auto exposure mode: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }

            res = uvc_set_exposure_abs(m_pDeviceHandle, (uint32_t)MapK4aExposureToLinux(newValue));
            if (res < 0)
            {
                LOG_ERROR("Failed to set exposure time abs: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else if (mode == K4A_COLOR_CONTROL_MODE_AUTO)
        {
            res = uvc_set_ae_mode(m_pDeviceHandle, UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY);
            if (res < 0)
            {
                LOG_ERROR("Failed to set auto exposure mode: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_BRIGHTNESS:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            res = uvc_set_brightness(m_pDeviceHandle, (int16_t)newValue);
            if (res < 0)
            {
                LOG_ERROR("Failed to set brightness: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_CONTRAST:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            res = uvc_set_contrast(m_pDeviceHandle, (uint16_t)newValue);
            if (res < 0)
            {
                LOG_ERROR("Failed to set contrast: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_SATURATION:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            res = uvc_set_saturation(m_pDeviceHandle, (uint16_t)newValue);
            if (res < 0)
            {
                LOG_ERROR("Failed to set saturation: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_SHARPNESS:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            res = uvc_set_sharpness(m_pDeviceHandle, (uint16_t)newValue);
            if (res < 0)
            {
                LOG_ERROR("Failed to set sharpness: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_WHITEBALANCE:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            res = uvc_set_white_balance_temperature_auto(m_pDeviceHandle, 0);
            if (res < 0)
            {
                LOG_ERROR("Failed to set auto white balance temperature mode: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }

            res = uvc_set_white_balance_temperature(m_pDeviceHandle, (uint16_t)newValue);
            if (res < 0)
            {
                LOG_ERROR("Failed to set white balance temerature: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else if (mode == K4A_COLOR_CONTROL_MODE_AUTO)
        {
            res = uvc_set_white_balance_temperature_auto(m_pDeviceHandle, 1);
            if (res < 0)
            {
                LOG_ERROR("Failed to set auto white balance temperature mode: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            res = uvc_set_backlight_compensation(m_pDeviceHandle, (uint16_t)newValue);
            if (res < 0)
            {
                LOG_ERROR("Failed to set backlight compensation: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_GAIN:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            res = uvc_set_gain(m_pDeviceHandle, (uint16_t)newValue);
            if (res < 0)
            {
                LOG_ERROR("Failed to set gain: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY:
        if (mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            if (newValue == 0)
            {
                // Even firmware does not able to disable power line frequncy control,
                // value zero is accepted.
                // Return failed unsupported value.
                LOG_ERROR("Can not disable Powerline Frequency Control", 0);
                return K4A_RESULT_FAILED;
            }

            res = uvc_set_power_line_frequency(m_pDeviceHandle, (uint8_t)newValue);
            if (res < 0)
            {
                LOG_ERROR("Failed to set powerline frequency: %s", uvc_strerror(res));
                return K4A_RESULT_FAILED;
            }
            else
            {
                m_using_60hz_power = (newValue == 2);
            }
        }
        else
        {
            LOG_ERROR("Invalid color control mode\n", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY:
    {
        LOG_WARNING("K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY is deprecated and does nothing.", 0);
    }
    break;
    default:
        LOG_ERROR("Unsupported control: %d\n", command);
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

// Callback function for when image objects are destroyed
static void uvc_camerareader_free_allocation(void *buffer, void *context)
{
    (void)context;
    allocator_free(buffer);
}

void UVCCameraReader::Callback(uvc_frame_t *frame)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_streaming && frame)
    {
        void *context = nullptr;
        k4a_image_t image = NULL;
        uint8_t *buffer = nullptr;
        size_t buffer_size = 0;
        int stride = 0;
        uint64_t framePTS = 0;
        uint64_t exposure_time = 0;
        uint32_t iso_speed = 0;
        uint32_t white_balance = 0;
        bool decodeMJPEG = false;
        bool drop_image = false;

        // Parse metadata
        size_t bufferLeft = (size_t)frame->metadata_bytes;
        if (bufferLeft >= sizeof(KSCAMERA_METADATA_ITEMHEADER))
        {
            PKSCAMERA_METADATA_ITEMHEADER pItem = (PKSCAMERA_METADATA_ITEMHEADER)frame->metadata;
            while (bufferLeft > 0)
            {
                switch (pItem->MetadataId)
                {
                case MetadataId_FrameAlignInfo:
                {
                    PKSCAMERA_CUSTOM_METADATA_FrameAlignInfo pFrameAlignInfo =
                        (PKSCAMERA_CUSTOM_METADATA_FrameAlignInfo)pItem;
                    framePTS = pFrameAlignInfo->FramePTS;
                }
                break;
                case MetadataId_CaptureStats:
                {
                    PKSCAMERA_METADATA_CAPTURESTATS pCaptureStats = (PKSCAMERA_METADATA_CAPTURESTATS)pItem;
                    if (pCaptureStats->Flags & KSCAMERA_METADATA_CAPTURESTATS_FLAG_EXPOSURETIME)
                    {
                        exposure_time = pCaptureStats->ExposureTime / 10; // hns to micro-second
                    }
                    if (pCaptureStats->Flags & KSCAMERA_METADATA_CAPTURESTATS_FLAG_ISOSPEED)
                    {
                        iso_speed = pCaptureStats->IsoSpeed;
                    }
                    if (pCaptureStats->Flags & KSCAMERA_METADATA_CAPTURESTATS_FLAG_WHITEBALANCE)
                    {
                        white_balance = pCaptureStats->WhiteBalance;
                    }
                }
                break;
                default:
                    break;
                }

                if (pItem->Size == 0)
                {
                    LOG_WARNING("frame metadata id %d has zero buffer size", pItem->MetadataId);
                    break;
                }

                bufferLeft -= (size_t)pItem->Size;
                if (bufferLeft < sizeof(KSCAMERA_METADATA_ITEMHEADER))
                {
                    break;
                }

                pItem = reinterpret_cast<PKSCAMERA_METADATA_ITEMHEADER>(reinterpret_cast<uint8_t *>(pItem) +
                                                                        pItem->Size);
            }
        }
        if (framePTS == 0)
        {
            // Drop 0 time stamped frame
            return;
        }

        if (m_input_image_format == K4A_IMAGE_FORMAT_COLOR_MJPG &&
            m_output_image_format == K4A_IMAGE_FORMAT_COLOR_BGRA32)
        {
            stride = (int)frame->width * 4;
            buffer_size = (size_t)stride * frame->height;
            decodeMJPEG = true;
        }
        else
        {
            stride = (int)frame->step;
            buffer_size = frame->data_bytes;
        }

        // Allocate K4A Color buffer
        buffer = allocator_alloc(ALLOCATION_SOURCE_COLOR, buffer_size);
        k4a_result_t result = K4A_RESULT_FROM_BOOL(buffer != NULL);

        if (K4A_SUCCEEDED(result))
        {
            if (decodeMJPEG)
            {
                // Decode MJPG into BRGA32
                result = DecodeMJPEGtoBGRA32((uint8_t *)frame->data, frame->data_bytes, buffer, buffer_size);
                if (K4A_FAILED(result))
                {
                    drop_image = true;
                }
            }
            else
            {
                // Copy to K4A buffer
                memcpy(buffer, frame->data, buffer_size);
            }
        }

        if (K4A_SUCCEEDED(result))
        {
            // The buffer size may be larger than the height * stride for some formats
            // so we must use image_create_from_buffer rather than image_create
            result = TRACE_CALL(image_create_from_buffer(m_output_image_format,
                                                         (int)m_width_pixels,
                                                         (int)m_height_pixels,
                                                         stride,
                                                         buffer,
                                                         buffer_size,
                                                         uvc_camerareader_free_allocation,
                                                         context,
                                                         &image));
        }
        else
        {
            // cleanup if there was an error
            allocator_free(buffer);
        }

        k4a_capture_t capture = NULL;
        if (K4A_SUCCEEDED(result))
        {
            result = TRACE_CALL(capture_create(&capture));
        }

        if (K4A_SUCCEEDED(result))
        {
            // Set metadata
            uint64_t ts = (uint64_t)frame->capture_time_finished.tv_sec * 1000000000;
            ts += (uint64_t)frame->capture_time_finished.tv_nsec;
            image_set_system_timestamp_nsec(image, ts);
            image_set_device_timestamp_usec(image, K4A_90K_HZ_TICK_TO_USEC(framePTS));
            image_set_exposure_usec(image, exposure_time);
            image_set_iso_speed(image, iso_speed);
            image_set_white_balance(image, white_balance);

            // Set image
            capture_set_color_image(capture, image);
        }

        if (!drop_image)
        {
            // Calback to color
            m_pCallback(result, capture, m_pCallbackContext);
        }

        if (image)
        {
            image_dec_ref(image);
        }

        if (capture)
        {
            // We guarantee that capture is valid for the duration of the callback function, if someone
            // needs it to live longer, then they need to add a ref
            capture_dec_ref(capture);
        }
    }
}

k4a_result_t
UVCCameraReader::DecodeMJPEGtoBGRA32(uint8_t *in_buf, const size_t in_size, uint8_t *out_buf, const size_t out_size)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, m_width_pixels * m_height_pixels * 4 > out_size);

    int decompressStatus = tjDecompress2(m_decoder,
                                         in_buf,
                                         (unsigned long)in_size,
                                         out_buf,
                                         (int)m_width_pixels,
                                         0, // pitch
                                         (int)m_height_pixels,
                                         TJPF_BGRA,
                                         TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE);

    if (decompressStatus != 0)
    {
        // This can happen when the host PC is not reading data off the camera fast enough. We also have the option to
        // move the use of libjpeg-turbo to a more recent version and use tjGetErrorCode() to get a better understanding
        // of the status returned.
        LOG_WARNING("MJPEG decode failed, dropping image: %d", decompressStatus);
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

// Returns exposure in 100us time base
int32_t UVCCameraReader::MapK4aExposureToLinux(int32_t K4aExposure_usec)
{
    // We map to the expected exposure then convert to 100us time base to ensure we roll over to the next exposure
    // setting in the same way Windows does.
    for (uint32_t x = 0; x < COUNTOF(device_exposure_mapping); x++)
    {
        int32_t mapped_exposure = device_exposure_mapping[x].exposure_mapped_50Hz_usec;
        if (m_using_60hz_power)
        {
            mapped_exposure = device_exposure_mapping[x].exposure_mapped_60Hz_usec;
        }

        if (K4aExposure_usec <= mapped_exposure)
        {
            return mapped_exposure / CONV_100USEC_TO_USEC;
        }
    }

    // Default to longest capture in the event mapping failed.
    return MAX_EXPOSURE(m_using_60hz_power) / CONV_100USEC_TO_USEC;
}

int32_t UVCCameraReader::MapLinuxExposureToK4a(int32_t LinuxExposure)
{
    LinuxExposure *= CONV_100USEC_TO_USEC; // Convert Linux 100us units to us.
    for (uint32_t x = 0; x < COUNTOF(device_exposure_mapping); x++)
    {
        int32_t mapped_exposure = device_exposure_mapping[x].exposure_mapped_50Hz_usec;
        if (m_using_60hz_power)
        {
            mapped_exposure = device_exposure_mapping[x].exposure_mapped_60Hz_usec;
        }

        if (LinuxExposure <= mapped_exposure)
        {
            return mapped_exposure;
        }
    }

    // Default to longest capture in the event mapping failed.
    return MAX_EXPOSURE(m_using_60hz_power);
}
