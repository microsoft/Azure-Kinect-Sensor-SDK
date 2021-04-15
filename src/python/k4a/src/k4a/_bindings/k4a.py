'''!
@file k4a.py

Defines Python _ctypes equivalent functions to those defined in k4a.h. 

Credit given to hexops's github contribution for the
_ctypes.Structure definitions and _ctypes function bindings.
https://github.com/hexops/Azure-Kinect-Python

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.
'''


import ctypes as _ctypes
import os.path as _os_path
import sys as _sys
import platform as _platform

from .k4atypes import *
from .k4atypes import _DeviceHandle, _CaptureHandle, _ImageHandle, \
    _TransformationHandle, _Calibration, _Float2, _Float3, \
    _memory_allocate_cb, _memory_destroy_cb


__all__ = []


# Load the k4a.dll.
try:
    _IS_WINDOWS = 'Windows' == _platform.system()
    _lib_dir = _os_path.join(_os_path.dirname(_os_path.dirname(__file__)), '_libs')

    if _IS_WINDOWS:
        _k4a_lib = _ctypes.CDLL(_os_path.join(_lib_dir, 'k4a.dll'))
    else:
        _k4a_lib = _ctypes.CDLL(_os_path.join(_lib_dir, 'libk4a.so'))

except Exception as ee:
    print("Failed to load library", ee)
    _sys.exit(1)


# Map _ctypes symbols to functions in the k4a.dll.

#K4A_EXPORT uint32_t k4a_device_get_installed_count(void);
k4a_device_get_installed_count = _k4a_lib.k4a_device_get_installed_count
k4a_device_get_installed_count.restype = _ctypes.c_uint32
k4a_device_get_installed_count.argtypes = None


#K4A_EXPORT k4a_status_t k4a_set_debug_message_handler(logging_message_cb *message_cb,
#                                                      void *message_cb_context,
#                                                      k4a_log_level_t min_level);
k4a_set_debug_message_handler = _k4a_lib.k4a_set_debug_message_handler
k4a_set_debug_message_handler.restype = EStatus
k4a_set_debug_message_handler.argtypes = (_ctypes.POINTER(logging_message_cb), _ctypes.c_void_p, _ctypes.c_int)


#K4A_EXPORT k4a_result_t k4a_set_allocator(k4a_memory_allocate_cb_t allocate, k4a_memory_destroy_cb_t free);
k4a_set_allocator = _k4a_lib.k4a_set_allocator
k4a_set_allocator.restype = EStatus
k4a_set_allocator.argtypes = (_memory_allocate_cb, _memory_destroy_cb,)


#K4A_EXPORT k4a_status_t k4a_device_open(uint32_t index, k4a_device_t *device_handle);
k4a_device_open = _k4a_lib.k4a_device_open
k4a_device_open.restype = EStatus
k4a_device_open.argtypes = (_ctypes.c_uint32, _ctypes.POINTER(_DeviceHandle))


#K4A_EXPORT void k4a_device_close(k4a_device_t device_handle);
k4a_device_close = _k4a_lib.k4a_device_close
k4a_device_close.restype = None
k4a_device_close.argtypes = (_DeviceHandle,)


#K4A_EXPORT k4a_wait_status_t k4a_device_get_capture(k4a_device_t device_handle,
#                                                    k4a_capture_t *capture_handle,
#                                                    int32_t timeout_in_ms);
k4a_device_get_capture = _k4a_lib.k4a_device_get_capture
k4a_device_get_capture.restype = EWaitStatus
k4a_device_get_capture.argtypes = (_DeviceHandle, _ctypes.POINTER(_CaptureHandle), _ctypes.c_int32)


#K4A_EXPORT k4a_wait_status_t k4a_device_get_imu_sample(k4a_device_t device_handle,
#                                                       k4a_imu_sample_t *imu_sample,
#                                                       int32_t timeout_in_ms);
k4a_device_get_imu_sample = _k4a_lib.k4a_device_get_imu_sample
k4a_device_get_imu_sample.restype = EWaitStatus
k4a_device_get_imu_sample.argtypes = (_DeviceHandle, _ctypes.POINTER(ImuSample), _ctypes.c_int32)


#K4A_EXPORT k4a_status_t k4a_capture_create(k4a_capture_t *capture_handle);
k4a_capture_create = _k4a_lib.k4a_capture_create
k4a_capture_create.restype = EStatus
k4a_capture_create.argtypes = (_ctypes.POINTER(_CaptureHandle),)


#K4A_EXPORT void k4a_capture_release(k4a_capture_t capture_handle);
k4a_capture_release = _k4a_lib.k4a_capture_release
k4a_capture_release.restype = None
k4a_capture_release.argtypes = (_CaptureHandle,)


#K4A_EXPORT void k4a_capture_reference(k4a_capture_t capture_handle);
k4a_capture_reference = _k4a_lib.k4a_capture_reference
k4a_capture_reference.restype = None
k4a_capture_reference.argtypes = (_CaptureHandle,)


#K4A_EXPORT k4a_image_t k4a_capture_get_color_image(k4a_capture_t capture_handle);
k4a_capture_get_color_image = _k4a_lib.k4a_capture_get_color_image
k4a_capture_get_color_image.restype = _ImageHandle
k4a_capture_get_color_image.argtypes=(_CaptureHandle,)


#K4A_EXPORT k4a_image_t k4a_capture_get_depth_image(k4a_capture_t capture_handle);
k4a_capture_get_depth_image = _k4a_lib.k4a_capture_get_depth_image
k4a_capture_get_depth_image.restype = _ImageHandle
k4a_capture_get_depth_image.argtypes=(_CaptureHandle,)


#K4A_EXPORT k4a_image_t k4a_capture_get_ir_image(k4a_capture_t capture_handle);
k4a_capture_get_ir_image = _k4a_lib.k4a_capture_get_ir_image
k4a_capture_get_ir_image.restype = _ImageHandle
k4a_capture_get_ir_image.argtypes=(_CaptureHandle,)


#K4A_EXPORT void k4a_capture_set_color_image(k4a_capture_t capture_handle, k4a_image_t image_handle);
k4a_capture_set_color_image = _k4a_lib.k4a_capture_set_color_image
k4a_capture_set_color_image.restype = None
k4a_capture_set_color_image.argtypes=(_CaptureHandle, _ImageHandle)


#K4A_EXPORT void k4a_capture_set_depth_image(k4a_capture_t capture_handle, k4a_image_t image_handle);
k4a_capture_set_depth_image = _k4a_lib.k4a_capture_set_depth_image
k4a_capture_set_depth_image.restype = None
k4a_capture_set_depth_image.argtypes=(_CaptureHandle, _ImageHandle)


#K4A_EXPORT void k4a_capture_set_ir_image(k4a_capture_t capture_handle, k4a_image_t image_handle);
k4a_capture_set_ir_image = _k4a_lib.k4a_capture_set_ir_image
k4a_capture_set_ir_image.restype = None
k4a_capture_set_ir_image.argtypes=(_CaptureHandle, _ImageHandle)


#K4A_EXPORT void k4a_capture_set_temperature_c(k4a_capture_t capture_handle, float temperature_c);
k4a_capture_set_temperature_c = _k4a_lib.k4a_capture_set_temperature_c
k4a_capture_set_temperature_c.restype = None
k4a_capture_set_temperature_c.argtypes=(_CaptureHandle, _ctypes.c_float)


#K4A_EXPORT float k4a_capture_get_temperature_c(k4a_capture_t capture_handle);
k4a_capture_get_temperature_c = _k4a_lib.k4a_capture_get_temperature_c
k4a_capture_get_temperature_c.restype = _ctypes.c_float
k4a_capture_get_temperature_c.argtypes=(_CaptureHandle,)


#K4A_EXPORT k4a_status_t k4a_image_create(k4a_image_format_t format,
#                                         int width_pixels,
#                                         int height_pixels,
#                                         int stride_bytes,
#                                         k4a_image_t *image_handle);
k4a_image_create = _k4a_lib.k4a_image_create
k4a_image_create.restype = EStatus
k4a_image_create.argtypes=(_ctypes.c_int, _ctypes.c_int, _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(_ImageHandle))



#K4A_EXPORT k4a_status_t k4a_image_create_from_buffer(k4a_image_format_t format,
#                                                     int width_pixels,
#                                                     int height_pixels,
#                                                     int stride_bytes,
#                                                     uint8_t *buffer,
#                                                     size_t buffer_size,
#                                                     k4a_memory_destroy_cb_t *buffer_release_cb,
#                                                     void *buffer_release_cb_context,
#                                                     k4a_image_t *image_handle);
k4a_image_create_from_buffer = _k4a_lib.k4a_image_create_from_buffer
k4a_image_create_from_buffer.restype = EStatus
k4a_image_create_from_buffer.argtypes=(
    _ctypes.c_int, _ctypes.c_int, _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(_ctypes.c_uint8),
    _ctypes.c_size_t, _memory_allocate_cb, _ctypes.c_void_p, _ctypes.POINTER(_ImageHandle))



#K4A_EXPORT uint8_t *k4a_image_get_buffer(k4a_image_t image_handle);
k4a_image_get_buffer = _k4a_lib.k4a_image_get_buffer
k4a_image_get_buffer.restype = _ctypes.POINTER(_ctypes.c_uint8)
k4a_image_get_buffer.argtypes=(_ImageHandle,)


#K4A_EXPORT size_t k4a_image_get_size(k4a_image_t image_handle);
k4a_image_get_size = _k4a_lib.k4a_image_get_size
k4a_image_get_size.restype = _ctypes.c_size_t
k4a_image_get_size.argtypes=(_ImageHandle,)


#K4A_EXPORT k4a_image_format_t k4a_image_get_format(k4a_image_t image_handle);
k4a_image_get_format = _k4a_lib.k4a_image_get_format
k4a_image_get_format.restype = EImageFormat
k4a_image_get_format.argtypes=(_ImageHandle,)


#K4A_EXPORT int k4a_image_get_width_pixels(k4a_image_t image_handle);
k4a_image_get_width_pixels = _k4a_lib.k4a_image_get_width_pixels
k4a_image_get_width_pixels.restype = _ctypes.c_int
k4a_image_get_width_pixels.argtypes=(_ImageHandle,)


#K4A_EXPORT int k4a_image_get_height_pixels(k4a_image_t image_handle);
k4a_image_get_height_pixels = _k4a_lib.k4a_image_get_height_pixels
k4a_image_get_height_pixels.restype = _ctypes.c_int
k4a_image_get_height_pixels.argtypes=(_ImageHandle,)


#K4A_EXPORT int k4a_image_get_stride_bytes(k4a_image_t image_handle);
k4a_image_get_stride_bytes = _k4a_lib.k4a_image_get_stride_bytes
k4a_image_get_stride_bytes.restype = _ctypes.c_int
k4a_image_get_stride_bytes.argtypes=(_ImageHandle,)


#K4A_EXPORT uint64_t k4a_image_get_device_timestamp_usec(k4a_image_t image_handle);
k4a_image_get_device_timestamp_usec = _k4a_lib.k4a_image_get_device_timestamp_usec
k4a_image_get_device_timestamp_usec.restype = _ctypes.c_uint64
k4a_image_get_device_timestamp_usec.argtypes=(_ImageHandle,)


#K4A_EXPORT uint64_t k4a_image_get_system_timestamp_nsec(k4a_image_t image_handle);
k4a_image_get_system_timestamp_nsec = _k4a_lib.k4a_image_get_system_timestamp_nsec
k4a_image_get_system_timestamp_nsec.restype = _ctypes.c_uint64
k4a_image_get_system_timestamp_nsec.argtypes=(_ImageHandle,)


#K4A_EXPORT uint64_t k4a_image_get_exposure_usec(k4a_image_t image_handle);
k4a_image_get_exposure_usec = _k4a_lib.k4a_image_get_exposure_usec
k4a_image_get_exposure_usec.restype = _ctypes.c_uint64
k4a_image_get_exposure_usec.argtypes=(_ImageHandle,)


#K4A_EXPORT uint32_t k4a_image_get_white_balance(k4a_image_t image_handle);
k4a_image_get_white_balance = _k4a_lib.k4a_image_get_white_balance
k4a_image_get_white_balance.restype = _ctypes.c_uint32
k4a_image_get_white_balance.argtypes=(_ImageHandle,)


#K4A_EXPORT uint32_t k4a_image_get_iso_speed(k4a_image_t image_handle);
k4a_image_get_iso_speed = _k4a_lib.k4a_image_get_iso_speed
k4a_image_get_iso_speed.restype = _ctypes.c_uint32
k4a_image_get_iso_speed.argtypes=(_ImageHandle,)


#K4A_EXPORT void k4a_image_set_device_timestamp_usec(k4a_image_t image_handle, uint64_t timestamp_usec);
k4a_image_set_device_timestamp_usec = _k4a_lib.k4a_image_set_device_timestamp_usec
k4a_image_set_device_timestamp_usec.restype = None
k4a_image_set_device_timestamp_usec.argtypes=(_ImageHandle, _ctypes.c_uint64)


#K4A_EXPORT void k4a_image_set_system_timestamp_nsec(k4a_image_t image_handle, uint64_t timestamp_nsec);
k4a_image_set_system_timestamp_nsec = _k4a_lib.k4a_image_set_system_timestamp_nsec
k4a_image_set_system_timestamp_nsec.restype = None
k4a_image_set_system_timestamp_nsec.argtypes=(_ImageHandle, _ctypes.c_uint64)


#K4A_EXPORT void k4a_image_set_exposure_usec(k4a_image_t image_handle, uint64_t exposure_usec);
k4a_image_set_exposure_usec = _k4a_lib.k4a_image_set_exposure_usec
k4a_image_set_exposure_usec.restype = None
k4a_image_set_exposure_usec.argtypes=(_ImageHandle, _ctypes.c_uint64)


#K4A_EXPORT void k4a_image_set_white_balance(k4a_image_t image_handle, uint32_t white_balance);
k4a_image_set_white_balance = _k4a_lib.k4a_image_set_white_balance
k4a_image_set_white_balance.restype = None
k4a_image_set_white_balance.argtypes=(_ImageHandle, _ctypes.c_uint32)


#K4A_EXPORT void k4a_image_set_iso_speed(k4a_image_t image_handle, uint32_t iso_speed);
k4a_image_set_iso_speed = _k4a_lib.k4a_image_set_iso_speed
k4a_image_set_iso_speed.restype = None
k4a_image_set_iso_speed.argtypes=(_ImageHandle, _ctypes.c_uint32)


#K4A_EXPORT void k4a_image_reference(k4a_image_t image_handle);
k4a_image_reference = _k4a_lib.k4a_image_reference
k4a_image_reference.restype = None
k4a_image_reference.argtypes=(_ImageHandle,)


#K4A_EXPORT void k4a_image_release(k4a_image_t image_handle);
k4a_image_release = _k4a_lib.k4a_image_release
k4a_image_release.restype = None
k4a_image_release.argtypes=(_ImageHandle,)


#K4A_EXPORT k4a_status_t k4a_device_start_cameras(k4a_device_t device_handle, const k4a_device_configuration_t *config);
k4a_device_start_cameras = _k4a_lib.k4a_device_start_cameras
k4a_device_start_cameras.restype = EWaitStatus
k4a_device_start_cameras.argtypes = (_DeviceHandle, _ctypes.POINTER(DeviceConfiguration))


#K4A_EXPORT void k4a_device_stop_cameras(k4a_device_t device_handle);
k4a_device_stop_cameras = _k4a_lib.k4a_device_stop_cameras
k4a_device_stop_cameras.restype = None
k4a_device_stop_cameras.argtypes=(_DeviceHandle,)


#K4A_EXPORT k4a_status_t k4a_device_start_imu(k4a_device_t device_handle);
k4a_device_start_imu = _k4a_lib.k4a_device_start_imu
k4a_device_start_imu.restype = EStatus
k4a_device_start_imu.argtypes = (_DeviceHandle,)


#K4A_EXPORT void k4a_device_stop_imu(k4a_device_t device_handle);
k4a_device_stop_imu = _k4a_lib.k4a_device_stop_imu
k4a_device_stop_imu.restype = None
k4a_device_stop_imu.argtypes = (_DeviceHandle,)


#K4A_EXPORT k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle,
#                                                        char *serial_number,
#                                                        size_t *serial_number_size);
k4a_device_get_serialnum = _k4a_lib.k4a_device_get_serialnum
k4a_device_get_serialnum.restype = EBufferStatus
k4a_device_get_serialnum.argtypes = (_DeviceHandle, 
    _ctypes.POINTER(_ctypes.c_char), _ctypes.POINTER(_ctypes.c_size_t))


#K4A_EXPORT k4a_status_t k4a_device_get_version(k4a_device_t device_handle, HardwareVersion *version);
k4a_device_get_version = _k4a_lib.k4a_device_get_version
k4a_device_get_version.restype = EStatus
k4a_device_get_version.argtypes = (_DeviceHandle, _ctypes.POINTER(HardwareVersion))


#K4A_EXPORT k4a_status_t k4a_device_get_color_control_capabilities(k4a_device_t device_handle,
#                                                                  k4a_color_control_command_t command,
#                                                                  bool *supports_auto,
#                                                                  int32_t *min_value,
#                                                                  int32_t *max_value,
#                                                                  int32_t *step_value,
#                                                                  int32_t *default_value,
#                                                                  k4a_color_control_mode_t *default_mode);
k4a_device_get_color_control_capabilities = _k4a_lib.k4a_device_get_color_control_capabilities
k4a_device_get_color_control_capabilities.restype = EStatus
k4a_device_get_color_control_capabilities.argtypes = (
    _DeviceHandle, _ctypes.c_int, _ctypes.POINTER(_ctypes.c_bool),
    _ctypes.POINTER(_ctypes.c_int32), _ctypes.POINTER(_ctypes.c_int32),
    _ctypes.POINTER(_ctypes.c_int32), _ctypes.POINTER(_ctypes.c_int32),
    _ctypes.POINTER(_ctypes.c_int) )


#K4A_EXPORT k4a_status_t k4a_device_get_color_control(k4a_device_t device_handle,
#                                                     k4a_color_control_command_t command,
#                                                     k4a_color_control_mode_t *mode,
#                                                     int32_t *value);
k4a_device_get_color_control = _k4a_lib.k4a_device_get_color_control
k4a_device_get_color_control.restype = EStatus
k4a_device_get_color_control.argtypes = (_DeviceHandle, _ctypes.c_int, _ctypes.POINTER(_ctypes.c_int), _ctypes.POINTER(_ctypes.c_int32))


#K4A_EXPORT k4a_status_t k4a_device_set_color_control(k4a_device_t device_handle,
#                                                     k4a_color_control_command_t command,
#                                                     k4a_color_control_mode_t mode,
#                                                     int32_t value);
k4a_device_set_color_control = _k4a_lib.k4a_device_set_color_control
k4a_device_set_color_control.restype = EStatus
k4a_device_set_color_control.argtypes = (_DeviceHandle, _ctypes.c_int, _ctypes.c_int, _ctypes.c_int32)


#K4A_EXPORT k4a_buffer_result_t k4a_device_get_raw_calibration(k4a_device_t device_handle,
#                                                              uint8_t *data,
#                                                              size_t *data_size);
k4a_device_get_raw_calibration = _k4a_lib.k4a_device_get_raw_calibration
k4a_device_get_raw_calibration.restype = EBufferStatus
k4a_device_get_raw_calibration.argtypes = (_DeviceHandle, _ctypes.POINTER(_ctypes.c_uint8), _ctypes.POINTER(_ctypes.c_size_t))


#K4A_EXPORT k4a_status_t k4a_device_get_calibration(k4a_device_t device_handle,
#                                                   const k4a_depth_mode_t depth_mode,
#                                                   const k4a_color_resolution_t color_resolution,
#                                                   k4a_calibration_t *calibration);
k4a_device_get_calibration = _k4a_lib.k4a_device_get_calibration
k4a_device_get_calibration.restype = EStatus
k4a_device_get_calibration.argtypes = (_DeviceHandle, _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(_Calibration))


#K4A_EXPORT k4a_status_t k4a_device_get_sync_jack(k4a_device_t device_handle,
#                                                 bool *sync_in_jack_connected,
#                                                 bool *sync_out_jack_connected);
k4a_device_get_sync_jack = _k4a_lib.k4a_device_get_sync_jack
k4a_device_get_sync_jack.restype = EStatus
k4a_device_get_sync_jack.argtypes = (_DeviceHandle, _ctypes.POINTER(_ctypes.c_bool), _ctypes.POINTER(_ctypes.c_bool))


#K4A_EXPORT k4a_status_t k4a_calibration_get_from_raw(char *raw_calibration,
#                                                     size_t raw_calibration_size,
#                                                     const k4a_depth_mode_t depth_mode,
#                                                     const k4a_color_resolution_t color_resolution,
#                                                     k4a_calibration_t *calibration);
k4a_calibration_get_from_raw = _k4a_lib.k4a_calibration_get_from_raw
k4a_calibration_get_from_raw.restype = EStatus
k4a_calibration_get_from_raw.argtypes = (_ctypes.POINTER(_ctypes.c_char), 
    _ctypes.c_size_t, _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(_Calibration))


#K4A_EXPORT k4a_status_t k4a_calibration_3d_to_3d(const k4a_calibration_t *calibration,
#                                                 const k4a_float3_t *source_point3d_mm,
#                                                 const k4a_calibration_type_t source_camera,
#                                                 const k4a_calibration_type_t target_camera,
#                                                 k4a_float3_t *target_point3d_mm);
k4a_calibration_3d_to_3d = _k4a_lib.k4a_calibration_3d_to_3d
k4a_calibration_3d_to_3d.restype = EStatus
k4a_calibration_3d_to_3d.argtypes = (
    _ctypes.POINTER(_Calibration), _ctypes.POINTER(_Float3),
    _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(_Float3))


#K4A_EXPORT k4a_status_t k4a_calibration_2d_to_3d(const k4a_calibration_t *calibration,
#                                                 const k4a_float2_t *source_point2d,
#                                                 const float source_depth_mm,
#                                                 const k4a_calibration_type_t source_camera,
#                                                 const k4a_calibration_type_t target_camera,
#                                                 k4a_float3_t *target_point3d_mm,
#                                                 int *valid);
k4a_calibration_2d_to_3d = _k4a_lib.k4a_calibration_2d_to_3d
k4a_calibration_2d_to_3d.restype = EStatus
k4a_calibration_2d_to_3d.argtypes = (
    _ctypes.POINTER(_Calibration), _ctypes.POINTER(_Float2), _ctypes.c_float,
    _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(_Float3), _ctypes.POINTER(_ctypes.c_int))


#K4A_EXPORT k4a_status_t k4a_calibration_3d_to_2d(const k4a_calibration_t *calibration,
#                                                 const k4a_float3_t *source_point3d_mm,
#                                                 const k4a_calibration_type_t source_camera,
#                                                 const k4a_calibration_type_t target_camera,
#                                                 k4a_float2_t *target_point2d,
#                                                 int *valid);
k4a_calibration_3d_to_2d = _k4a_lib.k4a_calibration_3d_to_2d
k4a_calibration_3d_to_2d.restype = EStatus
k4a_calibration_3d_to_2d.argtypes = (
    _ctypes.POINTER(_Calibration), _ctypes.POINTER(_Float3),
    _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(_Float2), _ctypes.POINTER(_ctypes.c_int))


#K4A_EXPORT k4a_status_t k4a_calibration_2d_to_2d(const k4a_calibration_t *calibration,
#                                                 const k4a_float2_t *source_point2d,
#                                                 const float source_depth_mm,
#                                                 const k4a_calibration_type_t source_camera,
#                                                 const k4a_calibration_type_t target_camera,
#                                                 k4a_float2_t *target_point2d,
#                                                 int *valid);
k4a_calibration_2d_to_2d = _k4a_lib.k4a_calibration_2d_to_2d
k4a_calibration_2d_to_2d.restype = EStatus
k4a_calibration_2d_to_2d.argtypes = (
    _ctypes.POINTER(_Calibration), _ctypes.POINTER(_Float2), _ctypes.c_float,
    _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(_Float2), _ctypes.POINTER(_ctypes.c_int))


#K4A_EXPORT k4a_status_t k4a_calibration_color_2d_to_depth_2d(const k4a_calibration_t *calibration,
#                                                             const k4a_float2_t *source_point2d,
#                                                             const k4a_image_t depth_image,
#                                                             k4a_float2_t *target_point2d,
#                                                             int *valid);
k4a_calibration_color_2d_to_depth_2d = _k4a_lib.k4a_calibration_color_2d_to_depth_2d
k4a_calibration_color_2d_to_depth_2d.restype = EStatus
k4a_calibration_color_2d_to_depth_2d.argtypes = (
    _ctypes.POINTER(_Calibration), _ctypes.POINTER(_Float2), _ImageHandle,
    _ctypes.POINTER(_Float2), _ctypes.POINTER(_ctypes.c_int))


#K4A_EXPORT k4a_transform_t k4a_transformation_create(const k4a_calibration_t *calibration);
k4a_transformation_create = _k4a_lib.k4a_transformation_create
k4a_transformation_create.restype = _TransformationHandle
k4a_transformation_create.argtypes = (_ctypes.POINTER(_Calibration),)


#K4A_EXPORT void k4a_transformation_destroy(k4a_transform_t transformation_handle);
k4a_transformation_destroy = _k4a_lib.k4a_transformation_destroy
k4a_transformation_destroy.restype = None
k4a_transformation_destroy.argtypes = (_TransformationHandle,)


#K4A_EXPORT k4a_status_t k4a_transformation_depth_image_to_color_camera(k4a_transform_t transformation_handle,
#                                                                       const k4a_image_t depth_image,
#                                                                       k4a_image_t transformed_depth_image);
k4a_transformation_depth_image_to_color_camera = _k4a_lib.k4a_transformation_depth_image_to_color_camera
k4a_transformation_depth_image_to_color_camera.restype = EStatus
k4a_transformation_depth_image_to_color_camera.argtypes = (
    _TransformationHandle, _ImageHandle, _ImageHandle)


#K4A_EXPORT k4a_status_t
#k4a_transformation_depth_image_to_color_camera_custom(k4a_transform_t transformation_handle,
#                                                      const k4a_image_t depth_image,
#                                                      const k4a_image_t custom_image,
#                                                      k4a_image_t transformed_depth_image,
#                                                      k4a_image_t transformed_custom_image,
#                                                      k4a_transformation_interpolation_type_t interpolation_type,
#                                                      uint32_t invalid_custom_value);
k4a_transformation_depth_image_to_color_camera_custom = _k4a_lib.k4a_transformation_depth_image_to_color_camera_custom
k4a_transformation_depth_image_to_color_camera_custom.restype = EStatus
k4a_transformation_depth_image_to_color_camera_custom.argtypes = (
    _TransformationHandle, _ImageHandle, _ImageHandle, _ImageHandle, _ImageHandle,
    _ctypes.c_int, _ctypes.c_uint32)


#K4A_EXPORT k4a_status_t k4a_transformation_color_image_to_depth_camera(k4a_transform_t transformation_handle,
#                                                                       const k4a_image_t depth_image,
#                                                                       const k4a_image_t color_image,
#                                                                       k4a_image_t transformed_color_image);
k4a_transformation_color_image_to_depth_camera = _k4a_lib.k4a_transformation_color_image_to_depth_camera
k4a_transformation_color_image_to_depth_camera.restype = EStatus
k4a_transformation_color_image_to_depth_camera.argtypes = (_TransformationHandle, _ImageHandle, _ImageHandle, _ImageHandle)


#K4A_EXPORT k4a_status_t k4a_transformation_depth_image_to_point_cloud(k4a_transform_t transformation_handle,
#                                                                      const k4a_image_t depth_image,
#                                                                      const k4a_calibration_type_t camera,
#                                                                      k4a_image_t xyz_image);
k4a_transformation_depth_image_to_point_cloud = _k4a_lib.k4a_transformation_depth_image_to_point_cloud
k4a_transformation_depth_image_to_point_cloud.restype = EStatus
k4a_transformation_depth_image_to_point_cloud.argtypes = (_TransformationHandle, _ImageHandle, _ctypes.c_int, _ImageHandle)
