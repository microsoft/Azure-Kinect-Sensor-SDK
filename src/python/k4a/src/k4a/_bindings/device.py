'''
device.py

Defines a Device class that opens a connection to an Azure Kinect device.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import ctypes as _ctypes
from os import linesep as _newline

from .k4atypes import _DeviceHandle, HardwareVersion, EStatus, EBufferStatus, \
    _EmptyClass, EColorControlCommand, EColorControlMode, ImuSample, \
    EWaitStatus, DeviceConfiguration, _CaptureHandle, EDepthMode, EColorResolution, \
    _Calibration, ImuSample

from .k4a import k4a_device_get_installed_count, k4a_device_open, \
    k4a_device_get_serialnum, k4a_device_get_version, \
    k4a_device_get_color_control_capabilities, k4a_device_close, \
    k4a_device_get_imu_sample, k4a_device_get_color_control, \
    k4a_device_start_cameras, k4a_device_stop_cameras, \
    k4a_device_start_imu, k4a_device_stop_imu, \
    k4a_device_set_color_control, k4a_device_get_raw_calibration, \
    k4a_device_get_sync_jack, k4a_device_get_capture, k4a_device_get_calibration

from .capture import Capture
from .calibration import Calibration


def _read_sync_jack_helper(device_handle:_DeviceHandle)->(bool, bool):
    
    retval = (False, False)
    
    # Read the sync jack.
    sync_in = _ctypes.c_bool(0)
    sync_out = _ctypes.c_bool(0)
    
    status = k4a_device_get_sync_jack(
        device_handle,
        _ctypes.byref(sync_in),
        _ctypes.byref(sync_out)
    )

    if status == EStatus.SUCCEEDED:
        retval = (sync_in.value, sync_out.value)

    return retval


class Device:

    _MAX_SERIAL_NUM_LENGTH = 32

    def __init__(self):
        self.__device_handle = None
        self._serial_number = None
        self._hardware_version = None
        self._color_ctrl_cap = None
        self._sync_out_connected = None
        self._sync_in_connected = None

    @staticmethod
    def get_device_count()->int:
        return k4a_device_get_installed_count()

    @staticmethod
    def open(device_index:int=0):

        device = Device()
        device.__device_handle = _DeviceHandle()
        device._serial_number = None
        device._hardware_version = HardwareVersion()
        device._color_ctrl_cap = _EmptyClass()

        # Open device and save device handle.
        status = k4a_device_open(
            device_index, 
            _ctypes.byref(device.__device_handle))

        if status != EStatus.SUCCEEDED:
            del device
            device = None
        else:
            # Get serial number.
            serial_number_size = _ctypes.c_ulonglong(Device._MAX_SERIAL_NUM_LENGTH)
            serial_number_buffer = _ctypes.create_string_buffer(
                Device._MAX_SERIAL_NUM_LENGTH)
            
            status_buffer = k4a_device_get_serialnum(
                device.__device_handle,
                serial_number_buffer,
                _ctypes.byref(serial_number_size))
            
            if status_buffer == EBufferStatus.SUCCEEDED:
                device._serial_number = str(serial_number_buffer.value)
            else:
                device._serial_number = str('')
            
            # Get hardware version.
            status = k4a_device_get_version(
                device.__device_handle,
                _ctypes.byref(device._hardware_version))

            if status != EStatus.SUCCEEDED:
                device._hardware_version = HardwareVersion()

            # Create a dictionary of color control capabilities.
            color_control_commands = [
                EColorControlCommand.BACKLIGHT_COMPENSATION,
                EColorControlCommand.BRIGHTNESS,
                EColorControlCommand.CONTRAST,
                EColorControlCommand.EXPOSURE_TIME_ABSOLUTE,
                EColorControlCommand.GAIN,
                EColorControlCommand.POWERLINE_FREQUENCY,
                EColorControlCommand.SATURATION,
                EColorControlCommand.SHARPNESS,
                EColorControlCommand.WHITEBALANCE
            ]

            supports_auto = _ctypes.c_bool(False)
            min_value = _ctypes.c_int32(0)
            max_value = _ctypes.c_int32(0)
            step_value = _ctypes.c_int32(0)
            default_value = _ctypes.c_int32(0)
            color_control_mode = _ctypes.c_int32(EColorControlMode.AUTO.value)

            for command in color_control_commands:

                status = k4a_device_get_color_control_capabilities(
                    device.__device_handle,
                    command,
                    _ctypes.byref(supports_auto),
                    _ctypes.byref(min_value),
                    _ctypes.byref(max_value),
                    _ctypes.byref(step_value),
                    _ctypes.byref(default_value),
                    _ctypes.byref(color_control_mode))

                if (status == EStatus.SUCCEEDED):
                    device._color_ctrl_cap.__dict__[command] = _EmptyClass()
                    device._color_ctrl_cap.__dict__[command].supports_auto = bool(supports_auto.value)
                    device._color_ctrl_cap.__dict__[command].min_value = int(min_value.value)
                    device._color_ctrl_cap.__dict__[command].max_value = int(max_value.value)
                    device._color_ctrl_cap.__dict__[command].step_value = int(step_value.value)
                    device._color_ctrl_cap.__dict__[command].default_value = int(default_value.value)
                    device._color_ctrl_cap.__dict__[command].default_mode = EColorControlMode(color_control_mode.value)
        
            # Read the sync jack.
            (device._sync_in_connected, device._sync_out_connected) = \
                _read_sync_jack_helper(device.__device_handle)

        return device

    def close(self):
        self.stop_cameras()
        self.stop_imu()
        k4a_device_close(self.__device_handle)

    # Allow syntax "with k4a.Depth.open() as device:"
    def __enter__(self):
        return self

    # Called automatically when exiting "with" block.
    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    # Prevent copying of a device handle.
    def __copy__(self):
        pass
    
    # Prevent deep copying of a device handle.
    def __deepcopy__(self, src):
        pass

    def __del__(self):
        # Ensure that handle is closed.
        self.close()

        del self.__device_handle
        del self._serial_number
        del self._hardware_version
        del self._color_ctrl_cap
        del self._sync_out_connected
        del self._sync_in_connected

        self.__device_handle = None
        self._serial_number = None
        self._hardware_version = None
        self._color_ctrl_cap = None
        self._sync_out_connected = None
        self._sync_in_connected = None

    def __str__(self):
        return ''.join([
            'serial_number=%s, ', _newline,
            'hardware_version=%s, ', _newline,
            'color_control_capabilities=%s, ', _newline,
            'sync_out_connected=%s, ', _newline,
            'sync_in_connected=%s']) % (
            self._serial_number,
            self._hardware_version.__str__(),
            self._color_ctrl_cap.__str__(),
            self._sync_out_connected,
            self._sync_in_connected)

    def get_imu_sample(self, timeout_ms:int=0):

        imu_sample = ImuSample()
        timeout = _ctypes.c_int(timeout_ms)
        status_wait = k4a_device_get_imu_sample(
            self.__device_handle,
            _ctypes.byref(imu_sample),
            timeout)

        if status_wait != EWaitStatus.SUCCEEDED:
            imu_sample = None

        return imu_sample

    def start_cameras(self, device_config:DeviceConfiguration)->EStatus:
        status = k4a_device_start_cameras(
            self.__device_handle,
            _ctypes.byref(device_config))
        
        return status

    def stop_cameras(self):
        k4a_device_stop_cameras(self.__device_handle)

    def start_imu(self)->EStatus:
        return k4a_device_start_imu(self.__device_handle)

    def stop_imu(self):
        k4a_device_stop_imu(self.__device_handle)

    def get_color_control(self, 
        color_ctrl_command:EColorControlCommand)->(int, EColorControlMode):

        retval = None
        retmode = None

        color_ctrl_value = _ctypes.c_int32(0)
        command = _ctypes.c_int(color_ctrl_command.value)
        mode = _ctypes.c_int32(EColorControlMode.MANUAL.value)
        
        status = k4a_device_get_color_control(
            self.__device_handle,
            color_ctrl_command,
            _ctypes.byref(mode),
            _ctypes.byref(color_ctrl_value))

        if status == EStatus.SUCCEEDED:
            retval = color_ctrl_value.value
            retmode = EColorControlMode(mode.value)

        return (retval, retmode)

    def set_color_control(
        self,
        color_ctrl_command:EColorControlCommand,
        color_ctrl_mode:EColorControlMode,
        color_ctrl_value:int)->EStatus:

        value = _ctypes.c_int32(color_ctrl_value)
        command = _ctypes.c_int(color_ctrl_command.value)
        mode = _ctypes.c_int(color_ctrl_mode.value)

        status = k4a_device_set_color_control(
            self.__device_handle,
            command,
            mode,
            value)

        return status

    def get_capture(self, timeout_ms:int)->Capture:

        capture = None

        # Get a capture handle.
        capture_handle = _CaptureHandle()
        timeout_in_ms = _ctypes.c_int32(timeout_ms)
        status = k4a_device_get_capture(
            self.__device_handle,
            _ctypes.byref(capture_handle),
            timeout_in_ms)

        if status == EStatus.SUCCEEDED:
            capture = Capture(capture_handle=capture_handle)

        return capture

    def get_imu_sample(self, timeout_ms:int)->ImuSample:

        imu_sample = ImuSample()

        wait_status = k4a_device_get_imu_sample(
            self.__device_handle,
            _ctypes.byref(imu_sample),
            _ctypes.c_int32(timeout_ms)
        )

        if wait_status != EWaitStatus.SUCCEEDED:
            imu_sample = None

        return imu_sample

    def get_raw_calibration(self)->bytearray:

        buffer = None

        # Get the size in bytes of the buffer that is required to
        # hold the raw calibration data.
        buffer_size_bytes = _ctypes.c_ulonglong(0)
        buffer_ptr = _ctypes.c_uint8(0)

        status = k4a_device_get_raw_calibration(
            self.__device_handle,
            _ctypes.byref(buffer_ptr),
            _ctypes.byref(buffer_size_bytes))
        
        if status != EBufferStatus.BUFFER_TOO_SMALL:
            return buffer

        print(buffer_size_bytes.value)

        # Create buffer of the correct size and get the raw calibration data.
        buffer = bytearray(buffer_size_bytes.value)
        cbuffer = (_ctypes.c_uint8 * buffer_size_bytes.value).from_buffer(buffer)
        cbufferptr = _ctypes.cast(cbuffer, _ctypes.POINTER(_ctypes.c_uint8))

        status = k4a_device_get_raw_calibration(
            self.__device_handle,
            cbufferptr,
            _ctypes.byref(buffer_size_bytes))

        if status != EBufferStatus.SUCCEEDED:
            buffer = None
        
        return buffer

    def get_calibration(self,
        depth_mode:EDepthMode,
        color_resolution:EColorResolution):

        calibration = None

        if not isinstance(depth_mode, EDepthMode):
            depth_mode = EDepthMode(depth_mode)

        if not isinstance(color_resolution, EColorResolution):
            color_resolution = EColorResolution(color_resolution)

        # Get ctypes calibration struct.
        _calibration = _Calibration()
        status = k4a_device_get_calibration(
            self.__device_handle,
            depth_mode,
            color_resolution,
            _ctypes.byref(_calibration))

        if status == EStatus.SUCCEEDED:
            # Wrap the ctypes struct into a non-ctypes class.
            calibration = Calibration(_calibration=_calibration)

        return calibration


#K4A_EXPORT k4a_status_t k4a_device_get_calibration(k4a_device_t device_handle,
#                                                   const k4a_depth_mode_t depth_mode,
#                                                   const k4a_color_resolution_t color_resolution,
#                                                   k4a_calibration_t *calibration);

    # Define properties and get/set functions. ############### 
    @property
    def serial_number(self):
        return self._serial_number

    @serial_number.deleter
    def serial_number(self):
        del self._serial_number

    @property
    def hardware_version(self):
        return self._hardware_version

    @hardware_version.deleter
    def hardware_version(self):
        del self._hardware_version

    @property
    def color_ctrl_cap(self):
        return self._color_ctrl_cap

    @color_ctrl_cap.deleter
    def color_ctrl_cap(self):
        del self._color_ctrl_cap

    @property
    def sync_in_connected(self):
        # Read the sync jack.
        (self._sync_in_connected, self._sync_out_connected) = \
            _read_sync_jack_helper(self.__device_handle)
        return self._sync_in_connected

    @sync_in_connected.deleter
    def sync_in_connected(self):
        del self._sync_in_connected

    @property
    def sync_out_connected(self):
        # Read the sync jack.
        (self._sync_in_connected, self._sync_out_connected) = \
            _read_sync_jack_helper(self.__device_handle)
        return self._sync_out_connected

    @sync_out_connected.deleter
    def sync_out_connected(self):
        del self._sync_out_connected
    # ###############
