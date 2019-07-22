# Depth Engine

The Azure Kinect Sensor SDK open source project does not include the depth engine which
converts raw sensor data in to a normalized depth map.

The depth engine is specific to the sensor in the Azure Kinect DK hardware.

The depth engine is not licensed under MIT as it contains proprietary code which is why it
can't be included in the Azure Kinect Sensor SDK project directly.

## Obtaining the Depth Engine

The depth engine is distributed as part of all Azure Kinect Sensor SDK binary releases.
If you are building the sensor SDK from source, you will need to obtain a copy from a binary
release to read data from a device.

### Windows

If you have run the windows binary installer (see the Azure Kinect DK public
documentation for details), you can get a copy of the depth engine from
`%Program Files%\Azure Kinect
SDK\sdk\windows-desktop\amd64\release\bin\depthengine_<major>_<minor>.dll`.

The depth engine must be in your %PATH% or located next to k4a.dll for the SDK
to decode frames from the Azure Kinect DK.

### Linux

The depth engine binary is part of the `libk4a<major>.<minor>` debian package.
If you have installed the `libk4a<major>.<minor>` debian package,
libdepthengine will be installed in your library path. If you have not (or can
not) you can extract libdepthengine from the download debian package found
[here](https://packages.microsoft.com/ubuntu/18.04/prod/pool/main/libk/).
