'''
test_k4atypes.py

Tests for the k4a types and enums.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import ctypes
from threading import Lock
    
import k4a


# Save capture to reuse in tests since it takes a while to get a capture from the device.
glb_capture = None
glb_color_format = None
glb_color_resolution = None
glb_depth_mode = None
glb_lock = Lock()


# Use for logging callback. But it doesn't work right now...
def glb_print_message(context:ctypes.c_void_p, 
                      level:k4a.ELogLevel, 
                      src_file:ctypes.POINTER(ctypes.c_char), 
                      src_line:ctypes.c_int, 
                      message:ctypes.POINTER(ctypes.c_char)):
    print(str(level) + " in " + str(src_file) + " at line " + str(src_line) + ": " + str(message))
    
    
# Used to get a capture from device, or a previously-captured capture.
def get_capture(device_handle:k4a._bindings.k4atypes._DeviceHandle,
                color_format:k4a.EImageFormat,
                color_resolution:k4a.EColorResolution,
                depth_mode:k4a.EDepthMode)->k4a._bindings.k4atypes._CaptureHandle:

    global glb_capture
    global glb_color_format
    global glb_color_resolution
    global glb_depth_mode

    capture = glb_capture

    if (capture is None or 
        glb_color_format != color_format or
        glb_color_resolution != color_resolution or
        glb_depth_mode != depth_mode):

        # Release any previous captures.
        if (capture is not None):
            k4a._bindings.k4a.k4a_capture_release(capture)
            capture = None

        # Start the cameras.
        device_config = k4a.DeviceConfiguration()
        device_config.color_format = color_format
        device_config.color_resolution = color_resolution
        device_config.depth_mode = depth_mode
        device_config.camera_fps = k4a.EFramesPerSecond.FPS_15
        device_config.synchronized_images_only = True
        device_config.depth_delay_off_color_usec = 0
        device_config.wired_sync_mode = k4a.EWiredSyncMode.STANDALONE
        device_config.subordinate_delay_off_master_usec = 0
        device_config.disable_streaming_indicator = False

        status = k4a._bindings.k4a.k4a_device_start_cameras(device_handle, ctypes.byref(device_config))
        if(k4a.K4A_SUCCEEDED(status)):

            # Get a capture. Ignore the retured status.
            capture = k4a._bindings.k4a._CaptureHandle()
            timeout_ms = ctypes.c_int32(1000)
            status = k4a._bindings.k4a.k4a_device_get_capture(
                device_handle,
                ctypes.byref(capture),
                timeout_ms
            )

        # Stop the cameras.
        k4a._bindings.k4a.k4a_device_stop_cameras(device_handle)

        glb_capture = capture
        glb_color_format = color_format
        glb_color_resolution = color_resolution
        glb_depth_mode = depth_mode

    return capture