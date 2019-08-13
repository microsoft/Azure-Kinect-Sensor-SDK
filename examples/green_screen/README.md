# Green Screen Example

The goal of the green screen example is to demonstrate best practices for using multiple Azure Kinect devices, with an emphasis on synchronization and calibration (the 'green screen' code is only a small portion of the logic). In particular, the green screen application showcases a physical limitation of the hardware and how it can be mostly addressed using another device.

## What does this application *do*?

The green screen example displays the scene as observed by one of the cameras (the 'main' camera). Using the camera's depth, it will paint over anything beyond its depth threshold with green, making it appear as if only the things in the foreground are there. It will fill in missing details with the 'backup' camera, if possible, resulting in a better green screen than the main camera could achieve alone.

## Why use two cameras? Isn't one good enough?

It's true that one camera can get you most of the way to a good solution in this case. However, if you only use one camera (either by using the single-camera mode, or just covering the backup camera), and something in the scene is closer to the camera than something else in the scene (for example, if you hold out an arm), you should see a "shadow" of green pixels on the object in the background near the edge of the obstructing object.

TODO one camera case is not yet supported

Why? The answer comes back to the physical construction of the Azure Kinect. The color camera and the depth camera are physically offset. Therefore, it's possible for the color camera to be able to see something that the depth camera cannot. If the depth camera cannot see a section of the image that the color camera can, when the depth image is transformed into the color camera space, segments of the transformed image that correspond to occluded images. However, this 'invalid' value (of 0) is also the value for pixels with unknown depth values. Unknown depth values can happen for several reasons... TODO elaborate.

TODO add images

## Installation instructions

This example requires OpenCV to be installed to build. TODO make this example, and OpenCV at large, optional in CMake

### OpenCV Installation Instructions

#### Linux

`sudo apt install libopencv-dev`

#### Windows

Our recommended way of getting OpenCV on Windows is by using `vcpkg`. First, get `vcpkg` on your computer by following the instructions [here](https://github.com/Microsoft/vcpkg) (make sure to hook up user-wide integration).

Then, run `vcpkg.exe install opencv:x64-windows`.

You should now be ready to build the SDK normally, which should build this example and put it in your build folder's `bin` directory.

## Running the program

Run the `green_screen` executable in the `bin` folder with no arguments for usage details and defaults. The first
argument is chessboard height, the second is chessboard width, and third is calibration board square side length in
millimeters. The next three are optional: the fourth is the threshold (in millimeters) past which the example paints the
image green, the fifth is the color camera exposure time in usec, and the sixth is the powerline frequency mode.

## A Note on Calibration

This program relies on transforming the backup camera's depth image into the color camera's space. This transformation requires knowing the transformation between the two cameras. To find that transformation, we must calibrate the cameras. This program relies on OpenCV's chessboard calibration functions. As a result, you should have a chessboard pattern to use while calibrating the cameras. If you don't have one, print one out. You can find a 9x6 one [here](https://docs.opencv.org/2.4/_downloads/pattern.png) (note that the 9x6 comes from the interior corners, not the number of squares). The example requires the calibration board to be in view for both devices' color cameras for many frames, so make sure it's visible to both cameras.

Also, DO NOT move the cameras during or after calibration! Changing that translation will cause the backup camera to provide inaccurate information.

## The Green Screen

Once calibration has completed, a new screen should pop up with a steady video stream. If it's working right, everything further from the camera than the threshold

# Potential reasons for failure

- The range of the depth camera is limited by the range of the depth camera, which depends on the depth mode. You may need to change the depth mode depending on how far you need to have your cutoff. See [here](https://docs.microsoft.com/en-us/azure/Kinect-dk/hardware-specification) for the specs. TODO table formatting on that page needs to be fixed.
