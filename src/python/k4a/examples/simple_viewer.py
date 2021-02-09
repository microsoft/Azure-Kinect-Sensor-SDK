'''
simple_viewer.py

A simple viewer to demonstrate the image capture capabilities of an Azure 
Kinect device using the Python API. This is not the fastest way to display
a sequence of images; this is only meant to show how to capture frames
in a sequence.

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.
'''

# This package is used for displaying the images.
# It is not part of the k4a package and is not a hard requirement.
import matplotlib.pyplot as plt

# This will import all the public symbols into the k4a namespace.
import k4a


def simple_viewer():
    
    # Open a device using the static function Device.open().
    device = k4a.Device.open()

    # In order to start capturing frames, need to start the cameras.
    # The start_cameras() function requires a device configuration which
    # specifies the modes in which to put the color and depth cameras.
    depth_modes = device.get_depth_modes()
    color_modes = device.get_color_modes()
    fps_modes = device.get_fps_modes()

    device_config = k4a.DeviceConfiguration(
        color_format=k4a.EImageFormat.COLOR_BGRA32,
        color_mode_id=color_modes[5].mode_id, # 2160P
        depth_mode_id=depth_modes[3].mode_id, # WFOV_2X2BINNED
        fps_mode_id=fps_modes[2].mode_id,     # FPS_15
        synchronized_images_only=True,
        depth_delay_off_color_usec=0,
        wired_sync_mode=k4a.EWiredSyncMode.STANDALONE,
        subordinate_delay_off_master_usec=0,
        disable_streaming_indicator=False)

    device.start_cameras(device_config)

    # Get a capture.
    # The -1 tells the device to wait forever until a capture is available.
    capture = device.get_capture(-1)

    # Open a matplotlib figure to display images.
    fig = plt.figure()
    ax = []
    ax.append(fig.add_subplot(1, 3, 1, label="Color"))
    ax.append(fig.add_subplot(1, 3, 2, label="Depth"))
    ax.append(fig.add_subplot(1, 3, 3, label="IR"))

    # The capture has the following fields that can be read right away:
    # color : the color image
    # depth : the depth image
    # ir : the ir image
    im = []
    im.append(ax[0].imshow(capture.color.data))
    im.append(ax[1].imshow(capture.depth.data, cmap='jet'))
    im.append(ax[2].imshow(capture.ir.data, cmap='gray'))

    ax[0].title.set_text('Color')
    ax[1].title.set_text('Depth')
    ax[2].title.set_text('IR')

    # Note: The data in the images is in BGRA planes, but the matplotlib
    # library expects them to be in RGBA. This results in an inverted color
    # display if not properly handled. The user can splice the planes as
    # appropriate or use opencv which has a function call to transform
    # BGRA into RGBA.

    while fig is not None:

        # Draw the figure with the images.
        plt.pause(.1)
        plt.draw()

        # Get a new capture.
        capture = device.get_capture(-1)
        if capture is None:
            del fig
            break

        # Update the images in the figures.
        im[0].set_data(capture.color.data)
        im[1].set_data(capture.depth.data)
        im[2].set_data(capture.ir.data)

        # There is no need to delete the capture since Python will take care of
        # that in the object's deleter.


    # There is no need to stop the cameras since the deleter will stop
    # the cameras, but it's still prudent to do it explicitly.
    device.stop_cameras()

    # There is no need to delete resources since Python will take care
    # of releasing resources in the objects' deleters.

if __name__ == '__main__':
    simple_viewer()
