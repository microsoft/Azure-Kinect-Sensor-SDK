# Depth Engine

The Azure Kinect Sensor SDK open source project does not include the depth engine which
converts raw sensor data in to a normalized depth map.

The depth engine is specific to the sensor in the Azure Kinect DK hardware.

The depth engine is not licensed under MIT as it contains propriety code which is why it
can't be included in the Azure Kinect Sensor SDK project directly.

## Obtaining the Depth Engine

The depth engine is distributed as part of all Azure Kinect Sensor SDK binary releases.
If you are building the sensor SDK from source, you will need to obtain a copy from a binary
release to read data from a device.

### Windows

If you have run the windows binary installer ([Azure Kinect SDK 1.0.2.msi](http://download.microsoft.com/download/B/4/D/B4D26442-DDA5-40C2-9913-3B23AE84A806/Azure%20Kinect%20SDK%201.0.2.msi)), you can get a copy of the depth engine from %Program Files%\Azure Kinect SDK\sdk\windows-desktop\amd64\release\bin\depthengine_1_0.dll.

The depth engine must be in your %PATH% or located next to k4a.dll for the SDK to decode
frames from the Azure Kinect DK.

### Linux

The depth engine binary is not yet available for Linux. It is coming soon as part of the 1.1 SDK release.
