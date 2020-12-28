'''
calibration.py

Defines a Calibration class that is a container for a device calibration.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import ctypes as _ctypes

from .k4atypes import _Calibration, EStatus, EDepthMode, EColorResolution, _Calibration

from .k4a import k4a_calibration_get_from_raw

class Calibration:

    def __init__(self, _calibration:_Calibration=None):
        self._calibration = _calibration
        if self._calibration is None:
            self._calibration = _Calibration()
    
    # Allow "with" syntax.
    def __enter__(self):
        return self

    # Called automatically when exiting "with" block.
    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def __str__(self):
        return self._calibration.__str__()

    @staticmethod
    def create_from_raw(
        raw_calibration:bytearray,
        depth_mode:EDepthMode,
        color_resolution:EColorResolution):

        calibration = None

        # Get the _Calibration struct from the raw buffer.
        if (isinstance(raw_calibration, bytearray) and
            isinstance(depth_mode, EDepthMode) and
            isinstance(color_resolution, EColorResolution)):

            buffer_size_bytes = _ctypes.c_ulonglong(len(raw_calibration))
            cbuffer = (_ctypes.c_uint8 * buffer_size_bytes.value).from_buffer(raw_calibration)
            cbufferptr = _ctypes.cast(cbuffer, _ctypes.POINTER(_ctypes.c_char))
            _calibration = _Calibration()

            status = k4a_calibration_get_from_raw(
                cbufferptr,
                buffer_size_bytes,
                depth_mode,
                color_resolution,
                _ctypes.byref(_calibration))

            if status == EStatus.SUCCEEDED:
                # Wrap the ctypes struct into a non-ctypes class.
                calibration = Calibration(_calibration)

        return calibration

    # Define properties and get/set functions. ############### 
    
    @property
    def depth_cam_cal(self):
        return self._calibration.depth_camera_calibration

    @property
    def color_cam_cal(self):
        return self._calibration.color_camera_calibration
    
    @property
    def extrinsics(self):
        return self._calibration.extrinsics

    @property
    def depth_mode(self):
        return self._calibration.depth_mode

    @property
    def color_resolution(self):
        return self._calibration.color_resolution

    # ###############