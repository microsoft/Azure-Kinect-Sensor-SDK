# Azure Kinect Undistort Example

## Introduction

The undistort example demonstrates how to undistort a depth map. It sets up a virtual pinhole camera with a 120 degree 
field of view. The choice of the virtual camera's parameters is arbitrary and can be changed. For every pixel of this 
virtual camera, we precompute the corresponding pixel coordinate in the real, distorted depth camera. At runtime, we 
use this precomputed lookup table to copy depth values from the distorted depth map to the undistorted depth map using 
nearest neighbor interpolation. This remapping function can, for example, also be accomplished using OpenCV's faster 
remap() function.

## Usage Info

```
undistort.exe <output file>
```

Example:

```
undistort.exe undistorted.csv
```
