'''
the_basics.py

A simple program that makes use of the Device, Capture, and Images classes.

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.
'''

# This will import all the public symbols into the k4a namespace.
import k4a


def the_basics():
    
    # Open a device using the static function Device.open().
    device = k4a.Device.open()
    if device is None:
        exit(-1)

    # Print the device serial number, hardware_version, and
    # color control capabilities.
    print(device.serial_number)
    print(device.hardware_version)
    print(device.color_ctrl_cap)

    # In order to start capturing frames, need to start the cameras.
    # The start_cameras() function requires a device configuration which
    # specifies the modes in which to put the color and depth cameras.
    # See DeviceConfiguration, EImageFormat, EColorResolution, EDepthMode,
    # EFramesPerSecond, and EWiredSyncMode.
    device_config = k4a.DeviceConfiguration(
        color_format=k4a.EImageFormat.COLOR_BGRA32,
        color_resolution=k4a.EColorResolution.RES_1080P,
        depth_mode=k4a.EDepthMode.WFOV_2X2BINNED,
        camera_fps=k4a.EFramesPerSecond.FPS_15,
        synchronized_images_only=True,
        depth_delay_off_color_usec=0,
        wired_sync_mode=k4a.EWiredSyncMode.STANDALONE,
        subordinate_delay_off_master_usec=0,
        disable_streaming_indicator=False)

    status = device.start_cameras(device_config)
    if status != k4a.EStatus.SUCCEEDED:
        exit(-1)

    # The IMUs can be started but only after starting the cameras.
    # Note that it returns an EWaitStatus rather than a EStatus.
    wait_status = device.start_imu()
    if wait_status != k4a.EWaitStatus.SUCCEEDED:
        exit(-1)

    imu_sample = device.get_imu_sample(-1)
    print(imu_sample)

    # Get a capture.
    # The -1 tells the device to wait forever until a capture is available.
    capture = device.get_capture(-1)

    # Print the color, depth, and IR image details and the temperature.
    print(capture.color)
    print(capture.depth)
    print(capture.ir)
    print(capture.temperature)

    # The capture object has fields for the color, depth, and ir images.
    # These are container classes; the actual image data is stored as a
    # numpy ndarray object in a field called data. Users can query these
    # fields directly.
    color_image = capture.color
    color_image_data = color_image.data

    # Get the image width and height in pixels, and stride in bytes.
    width_pixels = color_image.width_pixels
    height_pixels = color_image.height_pixels
    stride_bytes = color_image.stride_bytes

    # There is no need to stop the cameras since the deleter will stop
    # the cameras, but it's still prudent to do it explicitly.
    device.stop_cameras()
    device.stop_imu()

    # There is no need to delete resources since Python will take care
    # of releasing resources in the objects' deleters. To explicitly
    # delete the images, capture, and device objects, call del on them.

if __name__ == '__main__':
    the_basics()
