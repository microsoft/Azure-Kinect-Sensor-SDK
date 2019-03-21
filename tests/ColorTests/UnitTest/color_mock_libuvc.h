#ifndef COLOR_MOCK_LIBUVC_H
#define COLOR_MOCK_LIBUVC_H
#include <utcommon.h>
#include <libuvc/libuvc.h>

class MockLibUVC
{
public:
    MOCK_METHOD2(uvc_init, uvc_error_t(uvc_context_t **ctx, struct libusb_context *usb_ctx));
    MOCK_METHOD5(uvc_find_device,
                 uvc_error_t(uvc_context_t *ctx, uvc_device_t **dev, int vid, int pid, const char *sn));
    MOCK_METHOD2(uvc_open, uvc_error_t(uvc_device_t *dev, uvc_device_handle_t **devh));
    MOCK_METHOD6(uvc_get_stream_ctrl_format_size,
                 uvc_error_t(uvc_device_handle_t *devh,
                             uvc_stream_ctrl_t *ctrl,
                             enum uvc_frame_format format,
                             int width,
                             int height,
                             int fps));
    MOCK_METHOD5(uvc_start_streaming,
                 uvc_error_t(uvc_device_handle_t *devh,
                             uvc_stream_ctrl_t *ctrl,
                             uvc_frame_callback_t *cb,
                             void *user_ptr,
                             uint8_t flags));
    MOCK_METHOD1(uvc_stop_streaming, void(uvc_device_handle_t *devh));
    MOCK_METHOD1(uvc_close, void(uvc_device_handle_t *devh));
    MOCK_METHOD1(uvc_unref_device, void(uvc_device_t *dev));
    MOCK_METHOD1(uvc_exit, void(uvc_context_t *ctx));

    MOCK_METHOD3(uvc_get_ae_mode, uvc_error_t(uvc_device_handle_t *devh, uint8_t *mode, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_exposure_abs,
                 uvc_error_t(uvc_device_handle_t *devh, uint32_t *time, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_ae_priority,
                 uvc_error_t(uvc_device_handle_t *devh, uint8_t *priority, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_brightness,
                 uvc_error_t(uvc_device_handle_t *devh, int16_t *brightness, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_contrast,
                 uvc_error_t(uvc_device_handle_t *devh, uint16_t *contrast, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_saturation,
                 uvc_error_t(uvc_device_handle_t *devh, uint16_t *saturation, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_sharpness,
                 uvc_error_t(uvc_device_handle_t *devh, uint16_t *sharpness, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_white_balance_temperature_auto,
                 uvc_error_t(uvc_device_handle_t *devh, uint8_t *temperature_auto, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_white_balance_temperature,
                 uvc_error_t(uvc_device_handle_t *devh, uint16_t *temperature, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_backlight_compensation,
                 uvc_error_t(uvc_device_handle_t *devh, uint16_t *backlight_compensation, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_gain, uvc_error_t(uvc_device_handle_t *devh, uint16_t *gain, enum uvc_req_code req_code));
    MOCK_METHOD3(uvc_get_power_line_frequency,
                 uvc_error_t(uvc_device_handle_t *devh, uint8_t *power_line_frequency, enum uvc_req_code req_code));

    MOCK_METHOD2(uvc_set_ae_mode, uvc_error_t(uvc_device_handle_t *devh, uint8_t mode));
    MOCK_METHOD2(uvc_set_exposure_abs, uvc_error_t(uvc_device_handle_t *devh, uint32_t time));
    MOCK_METHOD2(uvc_set_ae_priority, uvc_error_t(uvc_device_handle_t *devh, uint8_t priority));
    MOCK_METHOD2(uvc_set_brightness, uvc_error_t(uvc_device_handle_t *devh, int16_t brightness));
    MOCK_METHOD2(uvc_set_contrast, uvc_error_t(uvc_device_handle_t *devh, uint16_t contrast));
    MOCK_METHOD2(uvc_set_saturation, uvc_error_t(uvc_device_handle_t *devh, uint16_t saturation));
    MOCK_METHOD2(uvc_set_sharpness, uvc_error_t(uvc_device_handle_t *devh, uint16_t sharpness));
    MOCK_METHOD2(uvc_set_white_balance_temperature_auto,
                 uvc_error_t(uvc_device_handle_t *devh, uint8_t temperature_auto));
    MOCK_METHOD2(uvc_set_white_balance_temperature, uvc_error_t(uvc_device_handle_t *devh, uint16_t temperature));
    MOCK_METHOD2(uvc_set_backlight_compensation,
                 uvc_error_t(uvc_device_handle_t *devh, uint16_t backlight_compensation));
    MOCK_METHOD2(uvc_set_gain, uvc_error_t(uvc_device_handle_t *devh, uint16_t gain));
    MOCK_METHOD2(uvc_set_power_line_frequency, uvc_error_t(uvc_device_handle_t *devh, uint8_t power_line_frequency));

    MOCK_METHOD1(uvc_strerror, const char *(uvc_error_t err));
};

#ifdef __cplusplus
extern "C" {
#endif
extern MockLibUVC *g_mockLibUVC;

void EXPECT_uvc_init(MockLibUVC &mockLibUVC);
void EXPECT_uvc_find_device(MockLibUVC &mockLibUVC, const char *serial_number);
void EXPECT_uvc_open(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_stream_ctrl_format_size(MockLibUVC &mockLibUVC);
void EXPECT_uvc_start_streaming(MockLibUVC &mockLibUVC);
void EXPECT_uvc_stop_streaming(MockLibUVC &mockLibUVC);
void EXPECT_uvc_close(MockLibUVC &mockLibUVC);
void EXPECT_uvc_unref_device(MockLibUVC &mockLibUVC);
void EXPECT_uvc_exit(MockLibUVC &mockLibUVC);

void EXPECT_uvc_get_ae_mode(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_exposure_abs(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_ae_priority(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_brightness(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_contrast(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_saturation(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_sharpness(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_white_balance_temperature_auto(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_white_balance_temperature(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_backlight_compensation(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_gain(MockLibUVC &mockLibUVC);
void EXPECT_uvc_get_power_line_frequency(MockLibUVC &mockLibUVC);

void EXPECT_uvc_set_ae_mode(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_exposure_abs(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_ae_priority(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_brightness(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_contrast(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_saturation(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_sharpness(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_white_balance_temperature_auto(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_white_balance_temperature(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_backlight_compensation(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_gain(MockLibUVC &mockLibUVC);
void EXPECT_uvc_set_power_line_frequency(MockLibUVC &mockLibUVC);

void EXPECT_uvc_strerror(MockLibUVC &mockLibUVC);
#ifdef __cplusplus
}
#endif

#endif // COLOR_MOCK_LIBUVC_H