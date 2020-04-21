# OpenCV compatibility example

## Introduction

The OpenCV compatibility example shows how to convert the Azure Kinect calibration type k4a_calibration_t into the corresponding data 
structures of OpenCV. The code demonstrates the use of k4a_calibration_3d_to_2d() to transform five 3D-coordinates of the color 
camera into pixel-coordinates of the depth camera. We then show how the same operation is accomplished using the corresponding 
OpenCV function projectPoints().

If the user has OpenCV installed, the OpenCV-specific code can be enabled by uncommenting the HAVE_OPENCV pound define. The 
OpenCV code has been tested using OpenCV 4.1.1.

## Usage Info

    opencv_example.exe

Example:

    opencv_example.exe
