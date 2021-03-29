'''
image_transformations.py

A simple program that transforms images from one camera coordinate to another.

Requirements:
Users should install the following python packages before using this module:
   matplotlib

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.
'''

# This package is used for displaying the images.
# It is not part of the k4a package and is not a hard requirement for k4a.
# Users need to install these packages in order to use this module.
import matplotlib.pyplot as plt
import numpy as np

# This will import all the public symbols into the k4a namespace.
import k4a


def plot_images(image1:k4a.Image, image2:k4a.Image, image3:k4a.Image, cmap:str=''):

    # Create figure and subplots.
    fig = plt.figure()
    ax = []
    ax.append(fig.add_subplot(1, 3, 1, label="Color"))
    ax.append(fig.add_subplot(1, 3, 2, label="Depth"))
    ax.append(fig.add_subplot(1, 3, 3, label="IR"))

    # Display images.
    im = []
    im.append(ax[0].imshow(image1.data))
    im.append(ax[1].imshow(image2.data, cmap='jet'))

    if len(cmap) == 0:
        im.append(ax[2].imshow(image3.data))
    else:
        im.append(ax[2].imshow(image3.data, cmap=cmap))


    # Create axes titles.
    ax[0].title.set_text('Color')
    ax[1].title.set_text('Depth')
    ax[2].title.set_text('Transformed Image')

    plt.show()


def image_transformations():
    
    # Open a device using the "with" syntax.
    with k4a.Device.open() as device:

        # In order to start capturing frames, need to start the cameras.
        # The start_cameras() function requires a device configuration which
        # specifies the modes in which to put the color and depth cameras.
        # For convenience, the k4a package pre-defines some configurations
        # for common usage of the Azure Kinect device, but the user can
        # modify the values to set the device in their preferred modes.
        device_config = k4a.DEVICE_CONFIG_BGRA32_1080P_WFOV_2X2BINNED_FPS15
        status = device.start_cameras(device_config)
        if status != k4a.EStatus.SUCCEEDED:
            raise IOError("Failed to start cameras.")

        # In order to create a Transformation class, we first need to get
        # a Calibration instance. Getting a calibration object needs the
        # depth mode and color camera resolution. Thankfully, this is part
        # of the device configuration used in the start_cameras() function.
        calibration = device.get_calibration(
            depth_mode=device_config.depth_mode,
            color_resolution=device_config.color_resolution)

        # Create a Transformation object using the calibration object as param.
        transform = k4a.Transformation(calibration)

        # Get a capture using the "with" syntax.
        with device.get_capture(-1) as capture:

            color = capture.color
            depth = capture.depth
            ir = capture.ir

            # Get a color image but transformed in the depth space.
            color_transformed = transform.color_image_to_depth_camera(depth, color)
            plot_images(color, depth, color_transformed)

            # Get a depth image but transformed in the color space.
            depth_transformed = transform.depth_image_to_color_camera(depth)
            plot_images(color, depth, depth_transformed, cmap='jet')

            # Get a depth image and custom image but transformed in the color 
            # space. Create a custom image. This must have EImageFormat.CUSTOM8
            # or EImageFormat.CUSTOM16 as the image_format, so create an 
            # entirely new Image and copy the IR data to that image.
            ir_custom = k4a.Image.create(
                k4a.EImageFormat.CUSTOM16,
                ir.width_pixels,
                ir.height_pixels,
                ir.stride_bytes)
            np.copyto(ir_custom.data, ir.data)

            depth_transformed, ir_transformed = transform.depth_image_to_color_camera_custom(
                depth, ir_custom, k4a.ETransformInterpolationType.LINEAR, 0)
            plot_images(color, depth_transformed, ir_transformed, cmap='gray')

    # There is no need to delete resources since Python will take care
    # of releasing resources in the objects' deleters. To explicitly
    # delete the images, capture, and device objects, call del on them.

if __name__ == '__main__':
    image_transformations()
