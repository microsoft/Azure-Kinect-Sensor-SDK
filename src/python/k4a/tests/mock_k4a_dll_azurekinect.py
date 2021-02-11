'''
mock_device_azurekinect.py

Mock an Azure Kinect device.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import ctypes
    
import k4a

# Mock function for opening a device.
opened_device_handles = []
max_num_devices = 4

def mock_k4a_device_get_installed_count()->int:
    global max_num_devices
    return max_num_devices

def mock_k4a_device_open(device_index, device_handle)->k4a.EStatus:
    global opened_device_handles
    global max_num_devices
    status = k4a.EStatus.FAILED

    if not isinstance(device_handle._obj, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if (len(opened_device_handles) <= max_num_devices and 
        device_handle._obj not in opened_device_handles):
        opened_device_handles.append(device_handle._obj)
        status = k4a.EStatus.SUCCEEDED
    
    return status

def mock_k4a_device_close(device_handle):
    global opened_device_handles

    if isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        if device_handle in opened_device_handles:
            opened_device_handles.remove(device_handle)

    return

# Mock function for getting device serial number
def mock_k4a_get_serialnum(device_handle, buffer, buffer_size)->k4a.EStatus:

    status = k4a.EStatus.FAILED

    if not isinstance(buffer, ctypes.c_char * 32):
        return status

    if not isinstance(buffer_size._obj, ctypes.c_ulonglong):
        return status

    if buffer_size._obj.value != 32:
        return status

    serial_number = "0123-4567890"
    buffer.value = bytes(serial_number, "utf-8")
    buffer_size._obj.value = len(serial_number)

    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting device hardware version.
def mock_k4a_get_version(device_handle, hardware_version)->k4a.EStatus:

    status = k4a.EStatus.FAILED

    if not isinstance(hardware_version._obj, k4a.HardwareVersion):
        return status

    hardware_version._obj.rgb.major = 0
    hardware_version._obj.rgb.minor = 1
    hardware_version._obj.rgb.iteration = 0

    hardware_version._obj.depth.major = 0
    hardware_version._obj.depth.minor = 1
    hardware_version._obj.depth.iteration = 0

    hardware_version._obj.audio.major = 0
    hardware_version._obj.audio.minor = 1
    hardware_version._obj.audio.iteration = 0

    hardware_version._obj.depth_sensor.major = 0
    hardware_version._obj.depth_sensor.minor = 1
    hardware_version._obj.depth_sensor.iteration = 0

    hardware_version._obj.firmware_build = 1
    hardware_version._obj.firmware_signature = 1
    
    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting device info.
def mock_k4a_get_device_info(device_handle, device_info)->k4a.EStatus:

    status = k4a.EStatus.FAILED
    temp_device_info = k4a.DeviceInfo()

    if not isinstance(device_info._obj, k4a.DeviceInfo):
        return status

    if device_info._obj.struct_size != temp_device_info.struct_size:
        return status

    if device_info._obj.struct_version != temp_device_info.struct_version:
        return status

    device_info._obj.struct_size = temp_device_info.struct_size
    device_info._obj.struct_version = temp_device_info.struct_version
    device_info._obj.vendor_id = 0x045E
    device_info._obj.device_id = 0x097C
    device_info._obj.capabilities = (
        k4a.EDeviceCapabilities.DEPTH | 
        k4a.EDeviceCapabilities.COLOR | 
        k4a.EDeviceCapabilities.IMU | 
        k4a.EDeviceCapabilities.MICROPHONE
        )
    
    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for starting cameras.
def mock_k4a_start_cameras(device_handle, device_config)->k4a.EStatus:

    status = k4a.EStatus.SUCCEEDED

    if not isinstance(device_config._obj, k4a.DeviceConfiguration):
        status = k4a.EStatus.FAILED

    if device_config._obj.color_format != k4a.EImageFormat.COLOR_BGRA32:
        status = k4a.EStatus.FAILED

    if device_config._obj.color_mode_id != 1:
        status = k4a.EStatus.FAILED

    if device_config._obj.depth_mode_id != 1:
        status = k4a.EStatus.FAILED

    if device_config._obj.fps_mode_id != 15:
        status = k4a.EStatus.FAILED

    return status

# Mock function for getting device serial number
def mock_k4a_get_serialnum(device_handle, buffer, buffer_size)->k4a.EStatus:

    status = k4a.EStatus.FAILED

    if not isinstance(buffer, ctypes.c_char * 32):
        return status

    if not isinstance(buffer_size._obj, ctypes.c_ulonglong):
        return status

    if buffer_size._obj.value != 32:
        return status

    serial_number = "0123-4567890"
    buffer.value = bytes(serial_number, "utf-8")
    buffer_size._obj.value = len(serial_number)

    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting mode count
def mock_k4a_get_mode_count(device_handle, mode_count)->k4a.EStatus:

    status = k4a.EStatus.FAILED

    if not isinstance(mode_count._obj, ctypes.c_long):
        return status

    mode_count._obj.value = 2

    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting color mode.
def mock_k4a_get_color_mode(device_handle, mode_index, color_mode)->k4a.EStatus:

    status = k4a.EStatus.FAILED

    if not isinstance(mode_index, int):
        return status

    if mode_index < 0 or mode_index >= 2:
        return status

    if not isinstance(color_mode._obj, k4a.ColorModeInfo):
        return status

    if color_mode._obj.struct_size != 40:
        return status

    if color_mode._obj.struct_version != k4a.K4A_ABI_VERSION:
        return status

    color_mode._obj._struct_size = 40
    color_mode._obj.struct_version = k4a.K4A_ABI_VERSION
    color_mode._obj.mode_id = 0
    color_mode._obj.width = 0
    color_mode._obj.height = 0
    color_mode._obj.native_format = k4a.EImageFormat.COLOR_MJPG
    color_mode._obj.horizontal_fov = 0
    color_mode._obj.vertical_fov = 0
    color_mode._obj.min_fps = 0
    color_mode._obj.max_fps = 0

    if mode_index == 1:
        color_mode._obj.mode_id = 1
        color_mode._obj.width = 512
        color_mode._obj.height = 256
        color_mode._obj.native_format = k4a.EImageFormat.COLOR_MJPG
        color_mode._obj.horizontal_fov = 120
        color_mode._obj.vertical_fov = 120
        color_mode._obj.min_fps = 5
        color_mode._obj.max_fps = 30

    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting depth mode.
def mock_k4a_get_depth_mode(device_handle, mode_index, depth_mode)->k4a.EStatus:

    status = k4a.EStatus.FAILED

    if not isinstance(mode_index, int):
        return status

    if mode_index < 0 or mode_index >= 2:
        return status

    if not isinstance(depth_mode._obj, k4a.DepthModeInfo):
        return status

    if depth_mode._obj.struct_size != 52:
        return status

    if depth_mode._obj.struct_version != k4a.K4A_ABI_VERSION:
        return status

    depth_mode._obj._struct_size = 52
    depth_mode._obj.struct_version = k4a.K4A_ABI_VERSION
    depth_mode._obj.mode_id = 0
    depth_mode._obj.width = 0
    depth_mode._obj.height = 0
    depth_mode._obj.passive_ir_only = False
    depth_mode._obj.native_format = k4a.EImageFormat.DEPTH16
    depth_mode._obj.horizontal_fov = 0
    depth_mode._obj.vertical_fov = 0
    depth_mode._obj.min_fps = 0
    depth_mode._obj.max_fps = 0
    depth_mode._obj.min_range = 0
    depth_mode._obj.max_range = 0

    if mode_index == 1:
        depth_mode._obj.mode_id = 1
        depth_mode._obj.width = 512
        depth_mode._obj.height = 256
        depth_mode._obj.passive_ir_only = False
        depth_mode._obj.native_format = k4a.EImageFormat.DEPTH16
        depth_mode._obj.horizontal_fov = 120
        depth_mode._obj.vertical_fov = 120
        depth_mode._obj.min_fps = 5
        depth_mode._obj.max_fps = 30
        depth_mode._obj.min_range = 100
        depth_mode._obj.max_range = 1500

    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting fps mode.
def mock_k4a_get_fps_mode(device_handle, mode_index, fps_mode)->k4a.EStatus:

    status = k4a.EStatus.FAILED

    if not isinstance(mode_index, int):
        return status

    if mode_index < 0 or mode_index >= 2:
        return status

    if not isinstance(fps_mode._obj, k4a.FPSModeInfo):
        return status

    if fps_mode._obj.struct_size != 16:
        return status

    if fps_mode._obj.struct_version != k4a.K4A_ABI_VERSION:
        return status

    fps_mode._obj._struct_size = 16
    fps_mode._obj.struct_version = k4a.K4A_ABI_VERSION
    fps_mode._obj.mode_id = 0
    fps_mode._obj.fps = 0

    if mode_index == 1:
        fps_mode._obj.mode_id = 15
        fps_mode._obj.fps = 15

    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting capture.
def mock_k4a_device_get_capture(device_handle, capture_handle, timeout_in_ms):

    status = k4a.EWaitStatus.FAILED

    if not isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if not isinstance(capture_handle._obj, k4a._bindings.k4atypes._CaptureHandle):
        return status

    if not isinstance(timeout_in_ms, ctypes.c_long):
        return status

    if device_handle not in opened_device_handles:
        return status

    status = k4a.EWaitStatus.SUCCEEDED
    return status

# Mock function for getting imu sample.
def mock_k4a_device_get_imu_sample(device_handle, imu_sample, timeout_in_ms):

    status = k4a.EWaitStatus.FAILED

    if not isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if not isinstance(imu_sample._obj, k4a._bindings.k4atypes.ImuSample):
        return status

    if not isinstance(timeout_in_ms, ctypes.c_long):
        return status

    if device_handle not in opened_device_handles:
        return status

    imu_sample._obj.temperature = 1.2
    imu_sample._obj.acc_sample.xyz.x = 1
    imu_sample._obj.acc_sample.xyz.y = 2
    imu_sample._obj.acc_sample.xyz.z = 3
    imu_sample._obj.acc_timestamp_usec = 1
    imu_sample._obj.gyro_sample.xyz.x = 4
    imu_sample._obj.gyro_sample.xyz.y = 5
    imu_sample._obj.gyro_sample.xyz.z = 6
    imu_sample._obj.gyro_timestamp_usec = 2

    status = k4a.EWaitStatus.SUCCEEDED
    return status

# Mock function for getting color control capabilities.
def mock_k4a_device_get_color_control_capabilities(device_handle, command, supports_auto,
    min_value, max_value, step_value, default_value, default_mode):

    status = k4a.EStatus.FAILED

    if not isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if not isinstance(command, ctypes.c_long):
        return status

    if not isinstance(supports_auto._obj, ctypes.c_bool):
        return status

    if not isinstance(min_value._obj, ctypes.c_long):
        return status
    
    if not isinstance(max_value._obj, ctypes.c_long):
        return status

    if not isinstance(step_value._obj, ctypes.c_long):
        return status

    if not isinstance(default_value._obj, ctypes.c_long):
        return status

    if not isinstance(default_mode._obj, ctypes.c_long):
        return status

    if command.value >= len(k4a.EColorControlCommand):
        return status

    # Write values just so they do not remain zeros.
    supports_auto._obj.value = True
    min_value._obj.value = 100
    max_value._obj.value = 9999
    step_value._obj.value = 1
    default_value._obj.value = 700
    default_mode._obj.value = k4a.EColorControlMode.MANUAL

    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting color control.
def mock_k4a_device_get_color_control(device_handle, command, mode, value):

    status = k4a.EStatus.FAILED

    if not isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if not isinstance(command, ctypes.c_long):
        return status

    if not isinstance(mode._obj, ctypes.c_long):
        return status

    if not isinstance(value._obj, ctypes.c_long):
        return status

    if command.value >= len(k4a.EColorControlCommand):
        return status

    mode._obj.value = k4a.EColorControlMode.MANUAL
    value._obj.value = 1000

    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for setting color control.
def mock_k4a_device_set_color_control(device_handle, command, mode, value):

    status = k4a.EStatus.FAILED

    if not isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if not isinstance(command, ctypes.c_long):
        return status

    if not isinstance(mode, ctypes.c_long):
        return status

    if not isinstance(value, ctypes.c_long):
        return status

    if command.value >= len(k4a.EColorControlCommand):
        return status
    
    # Just return success.
    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for setting color control.
def mock_k4a_device_get_sync_jack(device_handle, sync_in, sync_out):

    status = k4a.EStatus.FAILED

    if not isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if not isinstance(sync_in._obj, ctypes.c_bool):
        return status

    if not isinstance(sync_out._obj, ctypes.c_bool):
        return status

    sync_in._obj.value = False
    sync_out._obj.value = True
    
    # Just return success.
    status = k4a.EStatus.SUCCEEDED
    return status

# Mock function for getting raw calibration.
def mock_k4a_device_get_raw_calibration(device_handle, buffer, buffer_size_bytes):

    status = k4a.EStatus.FAILED

    if not isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if not isinstance(buffer_size_bytes._obj, ctypes.c_ulonglong):
        return status

    if buffer_size_bytes._obj.value < 1024*1024:
        buffer_size_bytes._obj.value = 1024*1024
        status = k4a.EBufferStatus.BUFFER_TOO_SMALL
    else:
        # Just return success.
        status = k4a.EBufferStatus.SUCCEEDED
    
    return status

# Mock function for getting raw calibration.
def mock_k4a_device_get_calibration(device_handle, depth_mode_id, color_mode_id, calibration):

    status = k4a.EStatus.FAILED

    if not isinstance(device_handle, k4a._bindings.k4atypes._DeviceHandle):
        return status

    if not isinstance(depth_mode_id, int):
        return status

    if not isinstance(color_mode_id, int):
        return status

    if not isinstance(calibration._obj, k4a._bindings.k4atypes._Calibration):
        return status

    # Just return success.
    status = k4a.EBufferStatus.SUCCEEDED
    return status