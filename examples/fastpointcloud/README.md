# Azure Kinect Fastpointcloud Example

## Introduction

The Azure Kinect Fastpointcloud example computes a 3d point cloud from a depth map. The example precomputes a lookup table 
by storing x- and y-scale factors for every pixel. At runtime, the 3d X-coordinate of a pixel in millimeters is derived 
by multiplying the pixel's depth value with the corresponding x-scale factor. The 3d Y-coordinate is obtained by 
multiplying with the y-scale factor.

This method represents an alternative to calling k4a_transformation_depth_image_to_point_cloud() and lends itself 
to efficient implementation on the GPU.

## Usage Info

```
fastpointcloud.exe <output file>
```

Example:

```
fastpointcloud.exe pointcloud.ply
```
