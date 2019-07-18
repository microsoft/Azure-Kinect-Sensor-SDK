#ifndef UVC_CAMERAREADER_H
#define UVC_CAMERAREADER_H
// k4a
#include <k4a/k4atypes.h>
#include <k4ainternal/color.h>

#include "color_priv.h"

// STL
#include <mutex>

// external
#include <libuvc/libuvc.h>
#include "turbojpeg.h"

class UVCCameraReader
{
public:
    UVCCameraReader();
    virtual ~UVCCameraReader();

    k4a_result_t Init(const char *serialNumber);

    k4a_result_t Start(const uint32_t width,
                       const uint32_t height,
                       const float fps,
                       const k4a_image_format_t imageFormat,
                       color_cb_stream_t *pCallback,
                       void *pCallbackContext);

    void Stop();

    void Shutdown();

    k4a_result_t GetCameraControlCapabilities(const k4a_color_control_command_t command,
                                              color_control_cap_t *capabilities);

    k4a_result_t GetCameraControl(const k4a_color_control_command_t command,
                                  k4a_color_control_mode_t *mode,
                                  int32_t *pValue);

    k4a_result_t SetCameraControl(const k4a_color_control_command_t command,
                                  const k4a_color_control_mode_t mode,
                                  int32_t newValue);

    void Callback(uvc_frame_t *frame);

private:
    bool IsInitialized()
    {
        return m_pContext && m_pDevice && m_pDeviceHandle;
    }

    k4a_result_t DecodeMJPEGtoBGRA32(uint8_t *in_buf, const size_t in_size, uint8_t *out_buf, const size_t out_size);

    int32_t MapK4aExposureToLinux(int32_t K4aExposure);
    int32_t MapLinuxExposureToK4a(int32_t LinuxExposure);

private:
    // Lock
    std::mutex m_mutex;

    // libUVC device and context
    uvc_context_t *m_pContext = nullptr;
    uvc_device_t *m_pDevice = nullptr;
    uvc_device_handle_t *m_pDeviceHandle = nullptr;
    bool m_streaming = false;
    bool m_using_60hz_power = true;

    // Image format cache
    uint32_t m_width_pixels;
    uint32_t m_height_pixels;
    k4a_image_format_t m_input_image_format;
    k4a_image_format_t m_output_image_format;

    // K4A stream callback
    color_cb_stream_t *m_pCallback = nullptr;
    void *m_pCallbackContext = nullptr;

    // MJPEG decoder
    tjhandle m_decoder = nullptr;
};

#endif // UVC_CAMERAREADER_H