#include "color_mock_libuvc.h"

using namespace testing;

#ifdef __cplusplus
extern "C" {
#endif

MockLibUVC *g_mockLibUVC = nullptr;

static uvc_context_t *g_uvc_context = (uvc_context_t *)0x0001;
static uvc_device_t *g_uvc_device = (uvc_device_t *)0x0002;
static uvc_device_handle_t *g_uvc_device_handle = (uvc_device_handle_t *)0x0004;

static bool g_opened = false;
static bool g_streaming = false;
static int g_device_ref_count = 0;

static uint8_t g_ae_mode = 8; // default is UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY
static uint32_t g_exposure_abs = 127;
static uint8_t g_ae_priority = 1;
static int16_t g_brightness = 128;
static uint16_t g_contrast = 5;
static uint16_t g_saturation = 32;
static uint16_t g_sharpness = 2;
static uint8_t g_white_balance_temperature_auto = 1;
static uint16_t g_white_balance_temperature = 4500;
static uint16_t g_backlight_compensation = 0;
static uint16_t g_gain = 0;
static uint8_t g_power_line_frequency = 2;

//
// Mock LibUVC
//
uvc_error_t uvc_init(uvc_context_t **ctx, struct libusb_context *usb_ctx)
{
    return g_mockLibUVC->uvc_init(ctx, usb_ctx);
}

void EXPECT_uvc_init(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_init(_, // uvc_context_t **ctx
                         _  // struct libusb_context *usb_ctx
                         ))
        .WillRepeatedly(Invoke([](uvc_context_t **ctx, struct libusb_context *usb_ctx) {
            uvc_error_t res = UVC_SUCCESS;
            (void)usb_ctx;

            if (ctx == nullptr)
            {
                res = UVC_ERROR_INVALID_PARAM;
            }

            if (res == UVC_SUCCESS)
            {
                *ctx = g_uvc_context;
            }

            return res;
        }));
}

uvc_error_t uvc_find_device(uvc_context_t *ctx, uvc_device_t **dev, int vid, int pid, const char *sn)
{
    return g_mockLibUVC->uvc_find_device(ctx, dev, vid, pid, sn);
}

void EXPECT_uvc_find_device(MockLibUVC &mockLibUVC, const char *serial_number)
{
    EXPECT_CALL(mockLibUVC,
                uvc_find_device(_, // uvc_context_t * ctx
                                _, // uvc_device_t * *dev
                                _, // int vid
                                _, // int pid
                                _  // const char *sn
                                ))
        .WillRepeatedly(Invoke([=](uvc_context_t *ctx, uvc_device_t **dev, int vid, int pid, const char *sn) {
            uvc_error_t res = UVC_SUCCESS;
            (void)sn;

            if (ctx == g_uvc_context)
            {
                if (dev == nullptr)
                {
                    res = UVC_ERROR_INVALID_PARAM;
                }
                else
                {
                    if (vid == 0x045e && pid == 0x097d && strcmp(sn, serial_number) == 0)
                    {
                        *dev = g_uvc_device;
                        g_device_ref_count++;
                    }
                    else
                    {
                        res = UVC_ERROR_NO_DEVICE;
                    }
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }

            return res;
        }));
}

uvc_error_t uvc_open(uvc_device_t *dev, uvc_device_handle_t **devh)
{
    return g_mockLibUVC->uvc_open(dev, devh);
}

void EXPECT_uvc_open(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_open(_, // uvc_device_t * dev
                         _  // uvc_device_handle_t * *devh
                         ))
        .WillRepeatedly(Invoke([](uvc_device_t *dev, uvc_device_handle_t **devh) {
            uvc_error_t res = UVC_SUCCESS;

            if (dev == g_uvc_device)
            {
                if (devh == nullptr)
                {
                    res = UVC_ERROR_INVALID_PARAM;
                }
                else
                {
                    *devh = g_uvc_device_handle;
                    g_opened = true;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }

            return res;
        }));
}

uvc_error_t uvc_get_stream_ctrl_format_size(uvc_device_handle_t *devh,
                                            uvc_stream_ctrl_t *ctrl,
                                            enum uvc_frame_format format,
                                            int width,
                                            int height,
                                            int fps)
{
    return g_mockLibUVC->uvc_get_stream_ctrl_format_size(devh, ctrl, format, width, height, fps);
}

void EXPECT_uvc_get_stream_ctrl_format_size(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_stream_ctrl_format_size(_, // uvc_device_handle_t * devh
                                                _, // uvc_stream_ctrl_t * ctrl
                                                _, // enum uvc_frame_format format
                                                _, // int width
                                                _, // int height
                                                _  // int fps
                                                ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh,
                                  uvc_stream_ctrl_t *ctrl,
                                  enum uvc_frame_format format,
                                  int width,
                                  int height,
                                  int fps) {
            uvc_error_t res = UVC_SUCCESS;

            if (devh == g_uvc_device_handle && ctrl != nullptr)
            {
                (void)format;
                (void)width;
                (void)height;
                (void)fps;
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }

            return res;
        }));
}

uvc_error_t uvc_start_streaming(uvc_device_handle_t *devh,
                                uvc_stream_ctrl_t *ctrl,
                                uvc_frame_callback_t *cb,
                                void *user_ptr,
                                uint8_t flags)
{
    return g_mockLibUVC->uvc_start_streaming(devh, ctrl, cb, user_ptr, flags);
}

void EXPECT_uvc_start_streaming(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_start_streaming(_, // uvc_device_handle_t * devh
                                    _, // uvc_stream_ctrl_t * ctrl
                                    _, // uvc_frame_callback_t * cb
                                    _, // void *user_ptr
                                    _  // uint8_t flags
                                    ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh,
                                  uvc_stream_ctrl_t *ctrl,
                                  uvc_frame_callback_t *cb,
                                  void *user_ptr,
                                  uint8_t flags) {
            uvc_error_t res = UVC_SUCCESS;

            if (devh == g_uvc_device_handle && ctrl != nullptr && cb != nullptr && user_ptr != nullptr)
            {
                (void)flags;
                g_streaming = true;
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }

            return res;
        }));
}

void uvc_stop_streaming(uvc_device_handle_t *devh)
{
    return g_mockLibUVC->uvc_stop_streaming(devh);
}

void EXPECT_uvc_stop_streaming(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_stop_streaming(_ // uvc_device_handle_t * devh
                                   ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh) {
            ASSERT_EQ(devh, g_uvc_device_handle);
            g_streaming = false;
        }));
}

void uvc_close(uvc_device_handle_t *devh)
{
    return g_mockLibUVC->uvc_close(devh);
}

void EXPECT_uvc_close(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_close(_ // uvc_device_handle_t * devh
                          ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh) {
            ASSERT_EQ(devh, g_uvc_device_handle);
            g_opened = false;
        }));
}

void uvc_unref_device(uvc_device_t *dev)
{
    return g_mockLibUVC->uvc_unref_device(dev);
}

void EXPECT_uvc_unref_device(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_unref_device(_ // uvc_device_t * dev
                                 ))
        .WillRepeatedly(Invoke([](uvc_device_t *dev) {
            ASSERT_EQ(dev, g_uvc_device);
            g_device_ref_count--;
            ASSERT_GE(g_device_ref_count, 0);
        }));
}

void uvc_exit(uvc_context_t *ctx)
{
    return g_mockLibUVC->uvc_exit(ctx);
}

void EXPECT_uvc_exit(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_exit(_ // uvc_context_t * ctx
                         ))
        .WillRepeatedly(Invoke([](uvc_context_t *ctx) { ASSERT_EQ(ctx, g_uvc_context); }));
}

uvc_error_t uvc_get_ae_mode(uvc_device_handle_t *devh, uint8_t *mode, enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_ae_mode(devh, mode, req_code);
}

void EXPECT_uvc_get_ae_mode(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_ae_mode(_, // uvc_device_handle_t * devh
                                _, // uint8_t * mode
                                _  // enum uvc_req_code req_code
                                ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint8_t *mode, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && mode != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *mode = g_ae_mode;
                    break;
                case UVC_GET_RES:
                    *mode = 9;
                    break;
                case UVC_GET_DEF:
                    *mode = 8;
                    break;
                case UVC_GET_MIN:
                case UVC_GET_MAX:
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_exposure_abs(uvc_device_handle_t *devh, uint32_t *time, enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_exposure_abs(devh, time, req_code);
}

void EXPECT_uvc_get_exposure_abs(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_exposure_abs(_, // uvc_device_handle_t * devh
                                     _, // uint32_t * time
                                     _  // enum uvc_req_code req_code
                                     ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint32_t *time, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && time != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *time = g_exposure_abs;
                    break;
                case UVC_GET_MIN:
                    *time = 3;
                    break;
                case UVC_GET_MAX:
                    *time = 16383;
                    break;
                case UVC_GET_RES:
                    *time = 1;
                    break;
                case UVC_GET_DEF:
                    *time = 127;
                    break;
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_ae_priority(uvc_device_handle_t *devh, uint8_t *priority, enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_ae_priority(devh, priority, req_code);
}

void EXPECT_uvc_get_ae_priority(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_ae_priority(_, // uvc_device_handle_t * devh
                                    _, // uint8_t * priority
                                    _  // enum uvc_req_code req_code
                                    ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint8_t *priority, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && priority != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *priority = g_ae_priority;
                    break;
                case UVC_GET_DEF:
                case UVC_GET_MIN:
                case UVC_GET_MAX:
                case UVC_GET_RES:
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_brightness(uvc_device_handle_t *devh, int16_t *brightness, enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_brightness(devh, brightness, req_code);
}

void EXPECT_uvc_get_brightness(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_brightness(_, // uvc_device_handle_t * devh
                                   _, // int16_t *brightness
                                   _  // enum uvc_req_code req_code
                                   ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, int16_t *brightness, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && brightness != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *brightness = g_brightness;
                    break;
                case UVC_GET_MIN:
                    *brightness = 0;
                    break;
                case UVC_GET_MAX:
                    *brightness = 255;
                    break;
                case UVC_GET_RES:
                    *brightness = 1;
                    break;
                case UVC_GET_DEF:
                    *brightness = 128;
                    break;
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_contrast(uvc_device_handle_t *devh, uint16_t *contrast, enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_contrast(devh, contrast, req_code);
}

void EXPECT_uvc_get_contrast(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_contrast(_, // uvc_device_handle_t * devh
                                 _, // uint16_t *contrast
                                 _  // enum uvc_req_code req_code
                                 ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t *contrast, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && contrast != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *contrast = g_contrast;
                    break;
                case UVC_GET_MIN:
                    *contrast = 0;
                    break;
                case UVC_GET_MAX:
                    *contrast = 10;
                    break;
                case UVC_GET_RES:
                    *contrast = 1;
                    break;
                case UVC_GET_DEF:
                    *contrast = 5;
                    break;
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_saturation(uvc_device_handle_t *devh, uint16_t *saturation, enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_saturation(devh, saturation, req_code);
}

void EXPECT_uvc_get_saturation(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_saturation(_, // uvc_device_handle_t * devh
                                   _, // uint16_t *saturation
                                   _  // enum uvc_req_code req_code
                                   ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t *saturation, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && saturation != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *saturation = g_saturation;
                    break;
                case UVC_GET_MIN:
                    *saturation = 0;
                    break;
                case UVC_GET_MAX:
                    *saturation = 63;
                    break;
                case UVC_GET_RES:
                    *saturation = 1;
                    break;
                case UVC_GET_DEF:
                    *saturation = 32;
                    break;
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_sharpness(uvc_device_handle_t *devh, uint16_t *sharpness, enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_sharpness(devh, sharpness, req_code);
}

void EXPECT_uvc_get_sharpness(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_sharpness(_, // uvc_device_handle_t * devh
                                  _, // uint16_t *sharpness
                                  _  // enum uvc_req_code req_code
                                  ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t *sharpness, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && sharpness != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *sharpness = g_sharpness;
                    break;
                case UVC_GET_MIN:
                    *sharpness = 0;
                    break;
                case UVC_GET_MAX:
                    *sharpness = 4;
                    break;
                case UVC_GET_RES:
                    *sharpness = 1;
                    break;
                case UVC_GET_DEF:
                    *sharpness = 2;
                    break;
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_white_balance_temperature_auto(uvc_device_handle_t *devh,
                                                   uint8_t *temperature_auto,
                                                   enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_white_balance_temperature_auto(devh, temperature_auto, req_code);
}

void EXPECT_uvc_get_white_balance_temperature_auto(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_white_balance_temperature_auto(_, // uvc_device_handle_t * devh
                                                       _, // uint8_t * temperature_auto
                                                       _  // enum uvc_req_code req_code
                                                       ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint8_t *temperature_auto, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && temperature_auto != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *temperature_auto = g_white_balance_temperature_auto;
                    break;
                case UVC_GET_DEF:
                    *temperature_auto = 1;
                    break;
                case UVC_GET_MIN:
                case UVC_GET_MAX:
                case UVC_GET_RES:
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_white_balance_temperature(uvc_device_handle_t *devh,
                                              uint16_t *temperature,
                                              enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_white_balance_temperature(devh, temperature, req_code);
}

void EXPECT_uvc_get_white_balance_temperature(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_white_balance_temperature(_, // uvc_device_handle_t * devh
                                                  _, // uint16_t *temperature
                                                  _  // enum uvc_req_code req_code
                                                  ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t *temperature, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && temperature != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *temperature = g_white_balance_temperature;
                    break;
                case UVC_GET_MIN:
                    *temperature = 2500;
                    break;
                case UVC_GET_MAX:
                    *temperature = 12500;
                    break;
                case UVC_GET_RES:
                    *temperature = 10;
                    break;
                case UVC_GET_DEF:
                    *temperature = 4500;
                    break;
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_backlight_compensation(uvc_device_handle_t *devh,
                                           uint16_t *backlight_compensation,
                                           enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_backlight_compensation(devh, backlight_compensation, req_code);
}

void EXPECT_uvc_get_backlight_compensation(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_backlight_compensation(_, // uvc_device_handle_t * devh
                                               _, // uint16_t *backlight_compensation
                                               _  // enum uvc_req_code req_code
                                               ))
        .WillRepeatedly(
            Invoke([](uvc_device_handle_t *devh, uint16_t *backlight_compensation, enum uvc_req_code req_code) {
                uvc_error_t res = UVC_SUCCESS;
                if (devh == g_uvc_device_handle && backlight_compensation != nullptr)
                {
                    switch (req_code)
                    {
                    case UVC_GET_CUR:
                        *backlight_compensation = g_backlight_compensation;
                        break;
                    case UVC_GET_MIN:
                        *backlight_compensation = 0;
                        break;
                    case UVC_GET_MAX:
                        *backlight_compensation = 1;
                        break;
                    case UVC_GET_RES:
                        *backlight_compensation = 1;
                        break;
                    case UVC_GET_DEF:
                        *backlight_compensation = 0;
                        break;
                    case UVC_GET_LEN:
                    case UVC_GET_INFO:
                    case UVC_SET_CUR:
                    default:
                        res = UVC_ERROR_INVALID_PARAM;
                        break;
                    }
                }
                else
                {
                    res = UVC_ERROR_INVALID_PARAM;
                }
                return res;
            }));
}

uvc_error_t uvc_get_gain(uvc_device_handle_t *devh, uint16_t *gain, enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_gain(devh, gain, req_code);
}

void EXPECT_uvc_get_gain(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_gain(_, // uvc_device_handle_t * devh
                             _, // uint16_t *gain
                             _  // enum uvc_req_code req_code
                             ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t *gain, enum uvc_req_code req_code) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle && gain != nullptr)
            {
                switch (req_code)
                {
                case UVC_GET_CUR:
                    *gain = g_gain;
                    break;
                case UVC_GET_MIN:
                    *gain = 0;
                    break;
                case UVC_GET_MAX:
                    *gain = 255;
                    break;
                case UVC_GET_RES:
                    *gain = 1;
                    break;
                case UVC_GET_DEF:
                    *gain = 0;
                    break;
                case UVC_GET_LEN:
                case UVC_GET_INFO:
                case UVC_SET_CUR:
                default:
                    res = UVC_ERROR_INVALID_PARAM;
                    break;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_get_power_line_frequency(uvc_device_handle_t *devh,
                                         uint8_t *power_line_frequency,
                                         enum uvc_req_code req_code)
{
    return g_mockLibUVC->uvc_get_power_line_frequency(devh, power_line_frequency, req_code);
}

void EXPECT_uvc_get_power_line_frequency(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_get_power_line_frequency(_, // uvc_device_handle_t * devh
                                             _, // uint8_t * power_line_frequency
                                             _  // enum uvc_req_code req_code
                                             ))
        .WillRepeatedly(
            Invoke([](uvc_device_handle_t *devh, uint8_t *power_line_frequency, enum uvc_req_code req_code) {
                uvc_error_t res = UVC_SUCCESS;
                if (devh == g_uvc_device_handle && power_line_frequency != nullptr)
                {
                    switch (req_code)
                    {
                    case UVC_GET_CUR:
                        *power_line_frequency = g_power_line_frequency;
                        break;
                    case UVC_GET_MIN:
                        *power_line_frequency = 1;
                        break;
                    case UVC_GET_MAX:
                        *power_line_frequency = 2;
                        break;
                    case UVC_GET_RES:
                        *power_line_frequency = 1;
                        break;
                    case UVC_GET_DEF:
                        *power_line_frequency = 2;
                        break;
                    case UVC_GET_LEN:
                    case UVC_GET_INFO:
                    case UVC_SET_CUR:
                    default:
                        res = UVC_ERROR_INVALID_PARAM;
                        break;
                    }
                }
                else
                {
                    res = UVC_ERROR_INVALID_PARAM;
                }
                return res;
            }));
}

uvc_error_t uvc_set_ae_mode(uvc_device_handle_t *devh, uint8_t mode)
{
    return g_mockLibUVC->uvc_set_ae_mode(devh, mode);
}

void EXPECT_uvc_set_ae_mode(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_ae_mode(_, // uvc_device_handle_t * devh
                                _  // uint8_t mode
                                ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint8_t mode) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (mode == 8 || mode == 1)
                {
                    g_ae_mode = mode;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_exposure_abs(uvc_device_handle_t *devh, uint32_t time)
{
    return g_mockLibUVC->uvc_set_exposure_abs(devh, time);
}

void EXPECT_uvc_set_exposure_abs(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_exposure_abs(_, // uvc_device_handle_t * devh
                                     _  // uint32_t time
                                     ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint32_t time) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (time >= 3 && time <= 16383)
                {
                    g_exposure_abs = time;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_ae_priority(uvc_device_handle_t *devh, uint8_t priority)
{
    return g_mockLibUVC->uvc_set_ae_priority(devh, priority);
}

void EXPECT_uvc_set_ae_priority(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_ae_priority(_, // uvc_device_handle_t * devh
                                    _  // uint8_t priority
                                    ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint8_t priority) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (priority <= 1)
                {
                    g_ae_priority = priority;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_brightness(uvc_device_handle_t *devh, int16_t brightness)
{
    return g_mockLibUVC->uvc_set_brightness(devh, brightness);
}

void EXPECT_uvc_set_brightness(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_brightness(_, // uvc_device_handle_t * devh
                                   _  // int16_t brightness
                                   ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, int16_t brightness) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (brightness >= 0 && brightness <= 255)
                {
                    g_brightness = brightness;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_contrast(uvc_device_handle_t *devh, uint16_t contrast)
{
    return g_mockLibUVC->uvc_set_contrast(devh, contrast);
}

void EXPECT_uvc_set_contrast(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_contrast(_, // uvc_device_handle_t * devh
                                 _  // uint16_t contrast
                                 ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t contrast) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (contrast <= 10)
                {
                    g_contrast = contrast;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_saturation(uvc_device_handle_t *devh, uint16_t saturation)
{
    return g_mockLibUVC->uvc_set_saturation(devh, saturation);
}

void EXPECT_uvc_set_saturation(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_saturation(_, // uvc_device_handle_t * devh
                                   _  // uint16_t saturation
                                   ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t saturation) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (saturation <= 63)
                {
                    g_saturation = saturation;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_sharpness(uvc_device_handle_t *devh, uint16_t sharpness)
{
    return g_mockLibUVC->uvc_set_sharpness(devh, sharpness);
}

void EXPECT_uvc_set_sharpness(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_sharpness(_, // uvc_device_handle_t * devh
                                  _  // uint16_t sharpness
                                  ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t sharpness) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (sharpness <= 4)
                {
                    g_sharpness = sharpness;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_white_balance_temperature_auto(uvc_device_handle_t *devh, uint8_t temperature_auto)
{
    return g_mockLibUVC->uvc_set_white_balance_temperature_auto(devh, temperature_auto);
}

void EXPECT_uvc_set_white_balance_temperature_auto(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_white_balance_temperature_auto(_, // uvc_device_handle_t * devh
                                                       _  // uint8_t temperature_auto
                                                       ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint8_t temperature_auto) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (temperature_auto <= 1)
                {
                    g_white_balance_temperature_auto = temperature_auto;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_white_balance_temperature(uvc_device_handle_t *devh, uint16_t temperature)
{
    return g_mockLibUVC->uvc_set_white_balance_temperature(devh, temperature);
}

void EXPECT_uvc_set_white_balance_temperature(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_white_balance_temperature(_, // uvc_device_handle_t * devh
                                                  _  // uint16_t temperature
                                                  ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t temperature) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (temperature >= 2500 && temperature <= 12500 && (temperature % 10) == 0)
                {
                    g_white_balance_temperature = temperature;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_backlight_compensation(uvc_device_handle_t *devh, uint16_t backlight_compensation)
{
    return g_mockLibUVC->uvc_set_backlight_compensation(devh, backlight_compensation);
}

void EXPECT_uvc_set_backlight_compensation(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_backlight_compensation(_, // uvc_device_handle_t * devh
                                               _  // uint16_t backlight_compensation
                                               ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t backlight_compensation) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (backlight_compensation <= 1)
                {
                    g_backlight_compensation = backlight_compensation;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_gain(uvc_device_handle_t *devh, uint16_t gain)
{
    return g_mockLibUVC->uvc_set_gain(devh, gain);
}

void EXPECT_uvc_set_gain(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_gain(_, // uvc_device_handle_t * devh
                             _  // uint16_t gain
                             ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint16_t gain) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (gain <= 255)
                {
                    g_gain = gain;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

uvc_error_t uvc_set_power_line_frequency(uvc_device_handle_t *devh, uint8_t power_line_frequency)
{
    return g_mockLibUVC->uvc_set_power_line_frequency(devh, power_line_frequency);
}

void EXPECT_uvc_set_power_line_frequency(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_set_power_line_frequency(_, // uvc_device_handle_t * devh
                                             _  // uint8_t power_line_frequency
                                             ))
        .WillRepeatedly(Invoke([](uvc_device_handle_t *devh, uint8_t power_line_frequency) {
            uvc_error_t res = UVC_SUCCESS;
            if (devh == g_uvc_device_handle)
            {
                if (power_line_frequency >= 1 && power_line_frequency <= 2)
                {
                    g_power_line_frequency = power_line_frequency;
                }
                else
                {
                    res = UVC_ERROR_NOT_SUPPORTED;
                }
            }
            else
            {
                res = UVC_ERROR_INVALID_PARAM;
            }
            return res;
        }));
}

const char *uvc_strerror(uvc_error_t err)
{
    return g_mockLibUVC->uvc_strerror(err);
}

void EXPECT_uvc_strerror(MockLibUVC &mockLibUVC)
{
    EXPECT_CALL(mockLibUVC,
                uvc_strerror(_ // uvc_error_t err
                             ))
        .WillRepeatedly(Invoke([](uvc_error_t err) {
            if (err == UVC_SUCCESS)
            {
                return "UVC_SUCCESS";
            }
            return "UVC_ERROR";
        }));
}

#ifdef __cplusplus
}
#endif
