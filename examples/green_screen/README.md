# Green Screen Example

The goal of the green screen example is to demonstrate best practices for using multiple Azure Kinect devices, with an emphasis on synchronization and calibration (the 'green screen' code is only a small portion of the logic). In particular, the green screen application showcases a physical limitation of the hardware and how it can be mostly addressed using another device.

## What does this application *do*?

The green screen example displays the scene as observed by one of the cameras (the 'main' camera). Using the camera's depth, it will paint over anything beyond its depth threshold with green, making it appear as if only the things in the foreground are there. It will fill in missing details with the 'backup' camera, if possible, resulting in a better green screen than the main camera could achieve alone.

## Why use two cameras? Isn't one good enough?

It's true that one camera can get you most of the way to a good solution in this case. However, if you only use one camera (either by using the single-camera mode, or just covering the backup camera), and something in the scene is closer to the camera than something else in the scene (for example, if you hold out an arm), you should see a "shadow" of green pixels on the object in the background near the edge of the obstructing object.

Why? The answer comes back to the physical construction of the Azure Kinect. The color camera and the depth camera are physically offset. Therefore, it's possible for the color camera to be able to see something that the depth camera cannot. If the depth camera cannot see a section of the image that the color camera can, when the depth image is transformed into the color camera space, segments of the transformed image that correspond to occluded images. These 'invalid' pixels are set to 0. However, using another camera's perspective, some of those missing values can be filled in using the secondary depth camera, which can (hopefully) see those parts of an object that are occluded from the main depth camera.

TODO add images

## Installation instructions

This example requires OpenCV to be installed to build. To ensure it will be built, ensure that OpenCV is found by adding `-DOpenCV_REQUIRED` to the `cmake` command.

### OpenCV Installation Instructions

#### Linux

`sudo apt install libopencv-dev`

#### Windows

Our recommended way of getting OpenCV on Windows is by installing pre-built libraries. There's even a PowerShell script that'll do much of the work for you in `scripts/install-opencv.ps1`. This will place OpenCV in your `C:\` folder. Next, you will need to add it to your PATH. You can do this using PowerShell's SetEnvironmentVariable command (TODO just do this in the script).

## Running the program

Run the `green_screen` executable in the `bin` folder with no arguments for usage details and defaults, and then fill in any customizations you might need.
## A Note on Calibration

This program relies on transforming the backup camera's depth image into the color camera's space. This transformation requires knowing the transformation between the two cameras. To find that transformation, we must calibrate the cameras. This program relies on OpenCV's chessboard calibration functions. As a result, you should have a chessboard pattern to use while calibrating the cameras. If you don't have one, print one out. You can find a 9x6 one [here](https://docs.opencv.org/2.4/_downloads/pattern.png) (note that the 9x6 comes from the interior corners, not the number of squares). The example requires the calibration board to be in view for both devices' color cameras for many frames, so make sure it's visible to both cameras.

Also, DO NOT move the cameras during or after calibration! Changing that translation will cause the backup camera to provide inaccurate information.

## The Green Screen

Once calibration has completed, a new screen should pop up with a steady video stream. If it's working right, everything further from the camera than the threshold will appear as green.

# Potential reasons for failure

- The range of the depth camera is limited by the range of the depth camera, which depends on the depth mode. You may need to change the depth mode depending on how far you need to have your cutoff. See [here](https://docs.microsoft.com/en-us/azure/Kinect-dk/hardware-specification) for the specs.

- If you're on Linux and you're getting strange libusb or libuvc errors when you try to use more than one camera, see [here](https://github.com/microsoft/Azure-Kinect-Sensor-SDK/issues/485).

- If you're on Windows and you're getting a popup about not being able to find an OpenCV dll, you may need to re-check your PATH to make sure it has that dll in it.
