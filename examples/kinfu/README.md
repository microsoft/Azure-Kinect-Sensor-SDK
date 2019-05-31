# Azure Kinect - OpenCV KinectFusion Example

## Introduction

The Azure Kinect - OpenCV KinectFusion example shows how to use the Azure Kinect SDK and device with the KinectFusion example from opencv_contrib's rgbd module (https://github.com/opencv/opencv_contrib/tree/master/modules/rgbd). The example demonstrates how to feed calibration and depth images from Azure Kinect APIs to the OpenCV KinectFusion module with undistortion. We render live KinectFusion results in this example as well as generate fused point cloud as a ply file when quitting application.

If the user has OpenCV/OpenCV_Contrib/VTK installed, the OpenCV-specific code in this example can be enabled by uncommenting the HAVE_OPENCV pound define. The OpenCV code in this example has been tested using OpenCV 4.1.0. The following steps are what we tried on Windows (the user can take this as a reference and configure the right path of OpenCV dependencies):
- Download VTK-8.2.0
- Follow the instruction from opencv_contrib to build opencv with extra modules (we used cmake-gui to generate sln file and built opencv and opencv_contrib modules with Visual Studio 2017, the user needs to configure the WITH_VTK and VTK_DIR in the cmake-gui before generating sln)
- The user needs to add opencv/opencv_contrib includes and lib dependencies in the kinfu_example.vcxproj file (AdditionalIncludeDirectories, AdditionalDependencies and AdditionalLibraryDirectories), then build the Examples.sln, the following is a list of dependencies we need for this example
    Includes:
        opencv-4.1\include
        opencv_contrib-4.1\modules\rgbd\include
        opencv_contrib-4.1\modules\viz\include
    Libs:
        opencv_core410d.lib
        opencv_calib3d410d.lib
        opencv_rgbd410d.lib
        opencv_highgui410d.lib
        opencv_viz410d.lib
        opencv_imgproc410d.lib
- You need to copy the opencv dlls as well as Azure Kinect SDK dlls to the kinfu_example.exe folder before running the application.

## Usage Info

    Usage: kinfu_example.exe
    Keys:   	q - Quit
        r - Reset KinFu
        v - Enable Viz Render Cloud (default is OFF, enable it slows down frame rate)
    The application will generate a kinectfusion_output.ply in the same folder when you quit use key q

Example:

    Usage: kinfu_example.exe
