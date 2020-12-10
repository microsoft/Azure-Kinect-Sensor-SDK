'''k4atypes.py

Defines Python _ctypes equivalent functions to those defined in k4a.h. 

Credit given to hexops's github contribution for the
_ctypes.Structure definitions and _ctypes function bindings.
https://github.com/hexops/Azure-Kinect-Python
'''

import ctypes as _ctypes

# Either k4a library is installed, or user is running from the repo
# Either way, the expectation is that we can import the from k4a.k4atypes.
try:
    from k4a.k4atypes import *
except Exception as e:
    import os
    import sys
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
    from k4a.k4atypes import *


# Map _ctypes symbols to functions in the k4a.dll.
# The dll should have been loaded in __init__.py which is run when this subpackage is imported.


#K4A_EXPORT k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle);
k4a_device_open = _k4a_dll.k4a_device_open
k4a_device_open.restype=k4a_result_t
k4a_device_open.argtypes=(_ctypes.c_uint32, _ctypes.POINTER(k4a_device_t))


#K4A_EXPORT k4a_result_t k4a_device_start_cameras(k4a_device_t device_handle, const k4a_device_configuration_t *config);
k4a_device_start_cameras = _k4a_dll.k4a_device_start_cameras
k4a_device_start_cameras.restype=_ctypes.c_int
k4a_device_start_cameras.argtypes=(k4a_device_t, _ctypes.POINTER(k4a_device_configuration_t))


"""
K4A_EXPORT k4a_result_t k4a_device_get_calibration(k4a_device_t device_handle,
                                                   const k4a_depth_mode_t depth_mode,
                                                   const k4a_color_resolution_t color_resolution,
                                                   k4a_calibration_t *calibration);
"""
k4a_device_get_calibration = _k4a_dll.k4a_device_get_calibration
k4a_device_get_calibration.restype=_ctypes.c_int
k4a_device_get_calibration.argtypes=(k4a_device_t, _ctypes.c_int, _ctypes.c_int, _ctypes.POINTER(k4a_calibration_t))


"""
K4A_EXPORT k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle,
                                                    k4a_capture_t *capture_handle,
                                                    int32_t timeout_in_ms);
"""
k4a_device_get_capture = _k4a_dll.k4a_device_get_capture
k4a_device_get_capture.restype=_ctypes.c_int
k4a_device_get_capture.argtypes=(k4a_device_t, _ctypes.POINTER(k4a_capture_t), _ctypes.c_int32)


#K4A_EXPORT void k4a_capture_release(k4a_capture_t capture_handle);
k4a_capture_release = _k4a_dll.k4a_capture_release
k4a_capture_release.argtypes=(k4a_capture_t,)


#K4A_EXPORT void k4a_image_release(k4a_image_t image_handle);
k4a_image_release = _k4a_dll.k4a_image_release
k4a_image_release.argtypes=(k4a_image_t,)


#K4A_EXPORT void k4a_device_stop_cameras(k4a_device_t device_handle);
k4a_device_stop_cameras = _k4a_dll.k4a_device_stop_cameras
k4a_device_stop_cameras.argtypes=(k4a_device_t,)


#K4A_EXPORT void k4a_device_close(k4a_device_t device_handle);
k4a_device_close = _k4a_dll.k4a_device_close
k4a_device_close.argtypes=(k4a_device_t,)


#K4A_EXPORT k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle, char *serial_number, size_t *serial_number_size);
k4a_device_get_serialnum = _k4a_dll.k4a_device_get_serialnum
k4a_device_get_capture.restype=k4a_buffer_result_t
k4a_device_close.argtypes=(k4a_device_t, _ctypes.c_char_p, _ctypes.c_ulonglong)


# Define symbols that will be exported with "from _k4a import *".
__all__ = [
]

del _ctypes