'''
device.py

Defines a Device class that opens a connection to an Azure Kinect device.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import ctypes as _ctypes

from .k4atypes import _DeviceHandle, HardwareVersion, EStatus, EBufferStatus, \
    _EmptyClass, EColorControlCommand, EColorControlMode, ImuSample, \
    EWaitStatus, DeviceConfiguration

from .k4a import k4a_device_get_installed_count, k4a_device_open, \
    k4a_device_get_serialnum, k4a_device_get_version, \
    k4a_device_get_color_control_capabilities, k4a_device_close, \
    k4a_device_get_imu_sample, k4a_device_start_cameras, k4a_device_start_imu,\
    k4a_device_stop_imu, k4a_device_get_color_control, \
    k4a_device_get_color_control, k4a_device_get_raw_calibration, \
    k4a_device_get_sync_jack


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

    # Define properties and get/set functions. ############### 
    @property
    def serial_number(self):
        return self._serial_number

    @serial_number.deleter
    def serial_number(self):
        del self._serial_number

    @property
    def hardware_version(self):
        return self._serial_number

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
    def device_handle(self):
        return None

    @device_handle.deleter
    def device_handle(self):
        self.close()
        del self.__device_handle

    @property
    def sync_in_connected(self):
        # Read the sync jack.
        (self._sync_in, self.sync_out) = _read_sync_jack_helper(self.__device_handle)
        return self._sync_in

    @sync_in_connected.deleter
    def sync_in_connected(self):
        del self._sync_in_connected

    @property
    def sync_out_connected(self):
        # Read the sync jack.
        (self._sync_in, self.sync_out) = _read_sync_jack_helper(self.__device_handle)
        return self._sync_out

    @sync_out_connected.deleter
    def sync_out_connected(self):
        del self._sync_out_connected

    # ###############

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
            raise IOError("Failed to open device at index {}.".format(
                device_index))
        
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
            ('backlight_comp', EColorControlCommand.BACKLIGHT_COMPENSATION),
            ('brightness', EColorControlCommand.BRIGHTNESS),
            ('contrast', EColorControlCommand.CONTRAST),
            ('exposure_time', EColorControlCommand.EXPOSURE_TIME_ABSOLUTE),
            ('gain', EColorControlCommand.GAIN),
            ('powerline_freq', EColorControlCommand.POWERLINE_FREQUENCY),
            ('saturation', EColorControlCommand.SATURATION),
            ('sharpness', EColorControlCommand.SHARPNESS),
            ('whitebalance', EColorControlCommand.WHITEBALANCE)
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
                command[1],
                _ctypes.byref(supports_auto),
                _ctypes.byref(min_value),
                _ctypes.byref(max_value),
                _ctypes.byref(step_value),
                _ctypes.byref(default_value),
                _ctypes.byref(color_control_mode))

            if (status == EStatus.SUCCEEDED):
                device._color_ctrl_cap.__dict__[command[0]] = _EmptyClass()
                device._color_ctrl_cap.__dict__[command[0]].supports_auto = bool(supports_auto.value)
                device._color_ctrl_cap.__dict__[command[0]].min_value = int(min_value.value)
                device._color_ctrl_cap.__dict__[command[0]].max_value = int(max_value.value)
                device._color_ctrl_cap.__dict__[command[0]].step_value = int(step_value.value)
                device._color_ctrl_cap.__dict__[command[0]].default_value = int(default_value.value)
                device._color_ctrl_cap.__dict__[command[0]].default_mode = EColorControlMode(color_control_mode.value)

        return device

    def close(self):
        self.stop_cameras()
        k4a_device_close(self.__device_handle)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

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

        # If user does not pass a device configuration, then start the cameras
        # with some default modes.
        if not device_config:
            raise IOError("start_cameras() requires a DeviceConfiguration object.")

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

    def get_color_control(
        color_ctrl_command:EColorControlCommand,
        color_ctrl_mode:EColorControlMode)->int:

        retval = None

        color_ctrl_value = _ctypes.c_int32(0)
        command = _ctypes.c_int(color_ctrl_command.value)
        mode = _ctypes.c_int(color_ctrl_mode.value)

        status = k4a_device_get_color_control(
            self.__device_handle,
            color_ctrl_command,
            _ctypes.byref(mode),
            _ctypes.byref(color_ctrl_value))

        if status == EStatus.SUCCEEDED:
            retval = color_ctrl_value.value

        return retval

    def set_color_control(
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
