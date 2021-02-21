'''!
@file device.py

Defines a Device class that opens a connection to an Azure Kinect device
and functions to communicate with the device.

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.

@mainpage Welcome

This documentation describes the Python API usage for the Azure Kinect Sensor
SDK.

For details about the Azure Kinect DK hardware and for more information about 
getting started with development please see https://azure.com/kinect.

@section api_languages API Languages
The Azure Kinect Sensor SDK is primarily a C API. This documentation covers the
Python wrapper extension to the API. For the most detailed documentation of API
behavior, see the documentation for the C functions that the Python classes 
wrap.

@section python_examples Examples
Refer to the Examples for example Python code to effectively use the Python 
API. Once the Python API package is installed, a simple "import k4a" will
give you access to all the classes and enums used in the API.

@example the_basics.py
@example image_transformations.py
@example simple_viewer.py
'''

import ctypes as _ctypes
from os import linesep as _newline
from math import log2

from .k4atypes import _DeviceHandle, HardwareVersion, EStatus, EBufferStatus, \
    EColorControlCommand, EColorControlMode, ImuSample, \
    EWaitStatus, EDeviceCapabilities, DeviceConfiguration, _CaptureHandle, \
    _Calibration, ImuSample, DeviceInfo, DepthModeInfo, ColorModeInfo, \
    FPSModeInfo, DeviceInfo, ColorControlCapability

from .k4a import k4a_device_get_installed_count, k4a_device_open, \
    k4a_device_get_serialnum, k4a_device_get_version, \
    k4a_device_get_color_control_capabilities, k4a_device_close, \
    k4a_device_get_imu_sample, k4a_device_get_color_control, \
    k4a_device_start_cameras, k4a_device_stop_cameras, \
    k4a_device_start_imu, k4a_device_stop_imu, \
    k4a_device_set_color_control, k4a_device_get_raw_calibration, \
    k4a_device_get_sync_jack, k4a_device_get_capture, k4a_device_get_calibration, \
    k4a_device_get_info, k4a_device_get_color_mode_count, k4a_device_get_color_mode, \
    k4a_device_get_depth_mode_count, k4a_device_get_depth_mode, \
    k4a_device_get_fps_mode_count, k4a_device_get_fps_mode

from .capture import Capture
from .calibration import Calibration


class Device:
    '''! A class that represents a connected Azure Kinect device.

    Property Name      | Type | R/W | Description
    ------------------ | ---- | --- | -----------------------------------------
    serial_number      | str  | R   | The unique serial number of the device.
    hardware_version   | HardwareVersion | R   | The hardware versions in the device.
    device_info        | DeviceInfo | R   | The device product IDs and capabilities.

    @remarks 
    - Use the static factory function open() to get a connected 
        Device instance.
    
    @remarks 
    - Do not use the Device() constructor to get a Device instance. It
        will return an object that is not connected to any device. Calling
        open() on that object will fail.
    '''

    _MAX_SERIAL_NUM_LENGTH = 32

    def __init__(self, device_index:int=0):
        self.__device_handle = None
        self._serial_number = None
        self._hardware_version = None
        self._device_info = None

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
        del self._device_info

        self.__device_handle = None
        self._serial_number = None
        self._hardware_version = None
        self._device_info = None

    def __str__(self):
        return ''.join([
            'serial_number=%s, ', _newline,
            'hardware_version=%s, ', _newline,
            'device_info=%s ']) % (
            self._serial_number,
            self._hardware_version.__str__(),
            self._device_info.__str__())

    @staticmethod
    def get_device_count()->int:
        '''! Gets the number of connected devices.

        @returns Number of sensors connected to the PC.

        @remarks 
        - This API counts the number of Azure Kinect devices connected
            to the host PC.
        '''
        return k4a_device_get_installed_count()

    @staticmethod
    def open(device_index:int=0):
        '''! Open an Azure Kinect device.
        
        @param device_index (int, optional): The index of the device to open,
            starting with 0. Default value is 0.
                
        @returns An instance of a Device class that has an open connection to the
            physical device. This instance grants exclusive access to the device.
        
        @remarks 
        - If unsuccessful, None is returned.
        
        @remarks 
        - When done with the device, close the device with close().
        '''
        device = Device()
        device.__device_handle = _DeviceHandle()
        device._serial_number = None
        device._hardware_version = HardwareVersion()
        device._device_info = DeviceInfo()

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
                
            # Get device info.
            status = k4a_device_get_info(
                device.__device_handle,
                _ctypes.byref(device._device_info))
                
            if status != EStatus.SUCCEEDED:
                device._device_info = DeviceInfo()

        return device

    def close(self):
        '''! Closes an Azure Kinect device.

        @remarks 
        - Once closed, the Device object is no longer valid.

        @remarks 
        - Before deleting a Device object, ensure that all Captures have
            been deleted to ensure that all memory is freed.
        '''
        self.stop_cameras()
        self.stop_imu()
        k4a_device_close(self.__device_handle)

    def _get_device_info(self)->DeviceInfo:
        '''! Get device information and capabilities.

        @returns A DeviceInfo instance, or None if failed.
        '''
        device_info = DeviceInfo()
        status = k4a_device_get_info(
            self.__device_handle,
            _ctypes.byref(device_info))

        if status != EStatus.SUCCEEDED:
            device_info = None

        return device_info

    def get_color_modes(self)->list:
        '''! Get a list of all available ColorModeInfo.
        
        \note The color mode with mode_id equal to 0 is reserved for "Color Off".

        @returns A list of all available ColorModeInfo, or None if failed.
        '''
        color_mode_infos = list()

        num_color_modes = _ctypes.c_int(0)
        status = k4a_device_get_color_mode_count(
            self.__device_handle,
            _ctypes.byref(num_color_modes))

        if status == EStatus.SUCCEEDED:
            for mode_index in range(num_color_modes.value):
                color_mode_info = ColorModeInfo()
                status = k4a_device_get_color_mode(
                    self.__device_handle,
                    mode_index,
                    _ctypes.byref(color_mode_info))

                if status == EStatus.SUCCEEDED:
                    color_mode_infos.append(color_mode_info)

        # If there are no color modes, return None.
        if (len(color_mode_infos) == 0):
            color_mode_infos = None

        return color_mode_infos

    def get_depth_modes(self)->list:
        '''! Get a list of all available DepthModeInfo.
        
        \note The depth mode with mode_id equal to 0 is reserved for "Depth Off".

        @returns A list of all available DepthModeInfo, or None if failed.
        '''
        depth_mode_infos = list()

        num_depth_modes = _ctypes.c_int(0)
        status = k4a_device_get_depth_mode_count(
            self.__device_handle,
            _ctypes.byref(num_depth_modes))

        if status == EStatus.SUCCEEDED:
            for mode_index in range(num_depth_modes.value):
                depth_mode_info = DepthModeInfo()
                status = k4a_device_get_depth_mode(
                    self.__device_handle,
                    mode_index,
                    _ctypes.byref(depth_mode_info))

                if status == EStatus.SUCCEEDED:
                    depth_mode_infos.append(depth_mode_info)

        # If there are no color modes, return None.
        if (len(depth_mode_infos) == 0):
            depth_mode_infos = None

        return depth_mode_infos

    def get_fps_modes(self)->list:
        '''! Get a list of all available FPSModeInfo.
        
        \note The fps mode with mode_id equal to 0 is reserved for "0 frames per second".

        @returns A list of all available FPSModeInfo, or None if failed.
        '''
        fps_mode_infos = list()

        num_fps_modes = _ctypes.c_int(0)
        status = k4a_device_get_fps_mode_count(
            self.__device_handle,
            _ctypes.byref(num_fps_modes))

        if status == EStatus.SUCCEEDED:
            for mode_index in range(num_fps_modes.value):
                fps_mode_info = FPSModeInfo()
                status = k4a_device_get_fps_mode(
                    self.__device_handle,
                    mode_index,
                    _ctypes.byref(fps_mode_info))

                if status == EStatus.SUCCEEDED:
                    fps_mode_infos.append(fps_mode_info)

         # If there are no color modes, return None.
        if (len(fps_mode_infos) == 0):
            fps_mode_infos = None

        return fps_mode_infos

    def get_capture(self, timeout_ms:int)->Capture:
        '''! Reads a sensor capture.

        @param timeout_ms (int): Specifies the time in milliseconds the 
            function should block waiting for the capture. If set to 0, the
            function will return without blocking. Passing a negative number
            will block indefinitely until data is available, the device is
            disconnected, or another error occurs.
                
        @returns An instance of a Capture class that contains image data from
            the sensors. If a capture is not available in the configured
            @p timeout_ms, then None is returned.
        
        @remarks 
        - Gets the next capture in the streamed sequence of captures 
            from the camera. If a new capture is not currently available, this
            function will block until the timeout is reached. The SDK will
            buffer at least two captures worth of data before dropping the
            oldest capture. Callers needing to capture all data need to ensure
            they read the data as fast as data is being produced on average.
            
        @remarks 
        - Upon successfully reading a capture this function will return
            a Capture instance. If a capture is not available in the configured 
            @p timeout_in_ms, then the API will return None.

        @remarks 
        - This function needs to be called while the device is in a
            running state; after start_cameras() is called and before 
            stop_cameras() is called.

        @remarks 
        - This function returns None when an internal problem is
            encountered, such as loss of the USB connection, inability to 
            allocate enough memory, and other unexpected issues. Any error
            encountered by this function signals the end of streaming data, 
            and caller should stop the stream using stop_cameras().
 
        @remarks 
        - If this function is waiting for data (non-zero timeout) when
            stop_cameras() or close() is called on another thread, this 
            function will encounter an error and return None.
        '''
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
        '''! Reads an IMU sample.

        @param timeout_ms (int): Specifies the time in milliseconds the 
            function should block waiting for the sample. If set to 0, the
            function will return without blocking. Passing a negative number
            will block indefinitely until data is available, the device is 
            disconnected, or another error occurs.
                
        @returns An instance of an ImuSample class that contains data from the
            IMU. If data is not available in the configured @p timeout_ms, then
            None is returned.

        @remarks 
        - Gets the next sample in the streamed sequence of IMU samples
            from the device. If a new sample is not currently available, this 
            function will block until the timeout is reached. The API will 
            buffer at least two camera capture intervals worth of samples 
            before dropping the oldest sample. Callers needing to capture all 
            data need to ensure they read the data as fast as the data is being
            produced on average.
 
        @remarks 
        - Upon successfully reading a sample this function will return 
            an ImuSample. If a sample is not available in the configured 
            @p timeout_ms, then the API will return None.
 
        @remarks 
        - This function needs to be called while the device is in a
            running state; after start_imu() is called and before stop_imu() is 
            called.
 
        @remarks 
        - This function returns None when an internal problem is
            encountered, such as loss of the USB connection, inability to
            allocate enough memory, and other unexpected issues. Any error
            returned by this function signals the end of streaming data, and 
            the caller should stop the stream using stop_imu().
 
        @remarks 
        - If this function is waiting for data (non-zero timeout) when
            stop_imu() or close() is called on another thread, this function
            will encounter an error and return None.
        '''
        imu_sample = ImuSample()

        wait_status = k4a_device_get_imu_sample(
            self.__device_handle,
            _ctypes.byref(imu_sample),
            _ctypes.c_int32(timeout_ms)
        )

        if wait_status != EWaitStatus.SUCCEEDED:
            imu_sample = None

        return imu_sample

    def start_cameras(self, device_config:DeviceConfiguration)->EStatus:
        '''! Starts color and depth camera capture.

        @param device_config (DeviceConfiguration): The configuration to run
            the device in. See DeviceConfiguration for the definition of
            the device configuration accepted by this function.

        @returns EStatus.SUCCEEDED if successful, EStatus.FAILED otherwise.

        @remarks
        - Individual sensors configured to run will now start to
            stream captured data.

        @remarks
        - It is not valid to call start_cameras() a second time on the
            same device until stop_cameras() has been called.

        @see DeviceConfiguration, stop_cameras
        '''
        status = k4a_device_start_cameras(
            self.__device_handle,
            _ctypes.byref(device_config))
        
        return status

    def stop_cameras(self):
        '''! Stops the color and depth camera capture.

        @remarks
        - The streaming of individual sensors stops as a result of this
            call. Once called, start_cameras() may be called again to resume
            sensor streaming.

        @remarks
        - This function may be called while another thread is blocking
            in get_capture(). Calling this function while another thread is in 
            that function will result in that function returning None.
        
        @see start_cameras
        '''
        k4a_device_stop_cameras(self.__device_handle)

    def start_imu(self)->EStatus:
        '''! Starts the IMU sample stream.

        @returns EStatus: EStatus.SUCCEEDED is returned on success,
            EStatus.FAILED on failure, and EStatus.UNSUPPORTED if the device
            does not support this operation.

        @remarks
        - Call this API to start streaming IMU data. It is not valid
            to call this function a second time on the same Device until 
            stop_imu() has been called.

        @remarks
        - This function is dependent on the state of the cameras. The
            color or depth camera must be started before the IMU. 
            EStatus.FAILED will be returned if neither of the cameras is
            running.
        '''
        return k4a_device_start_imu(self.__device_handle)

    def stop_imu(self):
        '''! Stops the IMU capture.

        @remarks
        - The streaming of the IMU stops as a result of this call. 
        Once called, start_imu() may be called again to resume sensor 
        streaming, so long as the cameras are running.

        @remarks
        - This function may be called while another thread is blocking
            in get_imu_sample(). Calling this function while another thread is
            in that function will result in that function returning a failure.
        '''
        k4a_device_stop_imu(self.__device_handle)

    def get_color_control_capabilities(self)->tuple:
        '''! Get the device color control capability.

        @returns (list, dict): A list of ColorControlCapability. It also
            returns the same information in a dictionary. If the device
            does not support a color camera, then (None, None) may be returned.
        
        @remarks Each control command may be set to manual or automatic. See the 
            definition of @ref EColorControlCommand on how to interpret the value
            for each command.
            
        @remarks Some control commands are only supported in manual mode. When a 
            command is in automatic mode, the value for that command is not valid.
        '''
        # Get device capabilities and check if it supports COLOR.
        if (self.device_info.capabilities.bitmap.bHasColor) == 0:
            return (None, None)

        ret_list = list()
        ret_dict = dict()

        # Get color control capabilities.
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

        for command in color_control_commands:

            c_command = _ctypes.c_int(command)
            supports_auto = _ctypes.c_bool(False)
            min_value = _ctypes.c_int32(0)
            max_value = _ctypes.c_int32(0)
            step_value = _ctypes.c_int32(0)
            default_value = _ctypes.c_int32(0)
            color_control_mode = _ctypes.c_int(0)

            status = k4a_device_get_color_control_capabilities(
                self.__device_handle,
                c_command,
                _ctypes.byref(supports_auto),
                _ctypes.byref(min_value),
                _ctypes.byref(max_value),
                _ctypes.byref(step_value),
                _ctypes.byref(default_value),
                _ctypes.byref(color_control_mode))

            if (status == EStatus.SUCCEEDED):

                capability = ColorControlCapability(
                    command=EColorControlCommand(c_command.value),
                    supports_auto=bool(supports_auto.value),
                    min_value=int(min_value.value),
                    max_value=int(max_value.value),
                    step_value=int(step_value.value),
                    default_value=int(default_value.value),
                    default_mode=EColorControlMode(color_control_mode.value))

                ret_list.append(capability)
                ret_dict[EColorControlCommand(command.value)] = capability

        if (len(ret_list) == 0):
            ret_list = None

        if (len(ret_dict) == 0):
            ret_dict = None

        return ret_list, ret_dict


    def get_color_control(self, 
        color_ctrl_command:EColorControlCommand)->(int, EColorControlMode):
        '''! Get the Azure Kinect color sensor control.

        @param color_ctrl_command (EColorControlCommand): Color sensor control
            command.

        @returns (int, EColorControlMode): A tuple where the first element is
            the value of the color sensor control requested, and the second
            element is the EColorControlMode that represents whether the
            command is in automatic or manual mode.

        @remarks 
        - The returned value is always written but is only valid when
            the returned EColorControlMode is EColorControlMode.MANUAL.

        @remarks 
        - Each control command may be set to manual or automatic. See 
            the definition of EColorControlCommand on how to interpret the
            value for each command.

        @remarks 
        - Some control commands are only supported in manual mode. When
            a command is in automatic mode, the value for that command is not 
            valid.

        @remarks 
        - Control values set on a device are reset only when the device
            is power cycled. The device will retain the settings even if the 
            device closed or the application is restarted. See the Device
            instance's color_ctrl_cap property for default values of the color
            control.
        '''

        retval = None
        retmode = None

        color_ctrl_value = _ctypes.c_int32(0)
        command = _ctypes.c_int(color_ctrl_command.value)
        mode = _ctypes.c_int32(EColorControlMode.MANUAL.value)
        
        status = k4a_device_get_color_control(
            self.__device_handle,
            command,
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
        '''! Set the Azure Kinect color sensor control value.

        @param color_ctrl_command (EColorControlCommand): Color sensor control
            command to set.

        @param color_ctrl_mode (EColorControlMode): Color sensor control mode
            to set. This mode represents whether the command is in automatic
            or manual mode.

        @param color_ctrl_value (int): The value to set the color sensor
            control. The value is only valid if @p color_ctrl_mode is set to
            EColorControlMode.MANUAL, and is otherwise ignored.

        @returns EStatus.SUCCEEDED if successful, EStatus.FAILED if unsuccessful,
            or EStatus.UNSUPPORTED if the device does not support this operation.

        @remarks 
        - Each control command may be set to manual or automatic. See 
            the definition of EColorControlCommand on how to interpret the
            @p value for each command.
        '''

        value = _ctypes.c_int32(color_ctrl_value)
        command = _ctypes.c_int(color_ctrl_command.value)
        mode = _ctypes.c_int(color_ctrl_mode.value)

        status = k4a_device_set_color_control(
            self.__device_handle,
            command,
            mode,
            value)

        return status

    def get_sync_jack(self)->(bool, bool):
        '''! Get the statuc of sync jacks (if available).

        @returns (bool, bool): If the sync in port is connected, the first
            value is True. Otherwise it is False. If the sync out port is
            connected, the second value is True. Otherwise it is False.
        '''
    
        sync_in_ret = False
        sync_out_ret = False
        
        # Read the sync jack.
        sync_in = _ctypes.c_bool(0)
        sync_out = _ctypes.c_bool(0)
        
        status = k4a_device_get_sync_jack(
            self.__device_handle,
            _ctypes.byref(sync_in),
            _ctypes.byref(sync_out)
        )

        if status == EStatus.SUCCEEDED:
            sync_in_ret = sync_in.value
            sync_out_ret = sync_out.value

        return sync_in_ret, sync_out_ret

    def get_raw_calibration(self)->bytearray:
        '''! Get the raw calibration blob for the entire Azure Kinect device.

        @returns bytearray: A byte array containing the raw calibration data.
            If this function fails to get the raw calibration data, then None
            is returned.

        @remarks 
        - If this function fails to get the raw calibration data, then
            None is returned.

        @see Calibration
        '''
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
        depth_mode_id:int,
        color_mode_id:int)->Calibration:
        '''! Get the camera calibration for the entire Azure Kinect device for
            a specific depth mode and color resolution.

        @param depth_mode_id (int): The mode in which the depth camera is
            operated.

        @param color_mode_id (int): The resolution in which the
            color camera is operated.

        @returns Calibration: A Calibration instance containing the calibration
            data for the specific depth mode and color resolution specified. If
            this function fails to get the calibration data, then None is
            returned.

        @remarks 
        - The calibration represents the data needed to transform 
            between the camera views and may be different for each operating 
            @p depth_mode_id and @p color_mode_id the device is configured to 
            operate in.

        @remarks 
        - The calibration object is used to instantiate the 
            Transformation class and functions.
        '''
        calibration = None

        # Get ctypes calibration struct.
        _calibration = _Calibration()
        status = k4a_device_get_calibration(
            self.__device_handle,
            depth_mode_id,
            color_mode_id,
            _ctypes.byref(_calibration))

        if status == EStatus.SUCCEEDED:
            # Wrap the ctypes struct into a non-ctypes class.
            calibration = Calibration(_calibration=_calibration)

        return calibration

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
    def device_info(self):
        return self._device_info

    @device_info.deleter
    def device_info(self):
        del self._device_info

    # ###############
