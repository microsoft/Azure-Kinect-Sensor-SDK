# Azure Kinect Python API(K4A)

Welcome to the Azure Kinect Python API! We hope you can use it to build many great applications and participate in the project. Don't be shy to ask questions, and provide feedback. See [Azure.com/Kinect](https://Azure.com/kinect) for device
info and available documentation.

## Introduction

**K4A** is a Python user mode API to read data from your Azure Kinect device.

## Why use K4A

K4A enables you to get the most out of your Azure Kinect camera. Features include:

* Depth camera access
* RGB camera access and control (e.g. exposure and white balance)
* Motion sensor (gyroscope and accelerometer) access
* Synchronized Depth-RGB camera streaming with configurable delay between cameras
* External device synchronization control with configurable delay offset between devices
* Camera frame meta-data access for image resolution, timestamp and temperature
* Device calibration data access

All image data is encapsulated in numpy arrays, allowing Python users to easily use the data in OpenCV
and other packages that work with numpy arrays.

## Documentation

API documentation will soon be made available.

## Building

K4A uses Python's setuptools to build a wheel file for distribution. 
For instructions on how to build this distributable, please see
[building](docs/building.md).

## Testing

For information on writing or running tests, please see [testing](docs/testing.md).

## Contribute

We welcome your contributions! Please see the [contribution guidelines](../../../CONTRIBUTING.md).

## Feedback

For SDK feedback or to report a bug, please file a [GitHub Issue](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/issues). For general suggestions or ideas, visit our [feedback forum](https://aka.ms/azurekinectfeedback).

## Sample Code

Sample Python code that uses the Python API can be found in the examples/ folder. See [examples](docs/examples.md).

## Q&A

Welcome to the [Q&A](kinect-qa.md) corner!

## Join Our Developer Program

Complete your developer profile [here](https://aka.ms/iwantmr) to get connected with our Mixed Reality Developer Program. You will receive the latest on our developer tools, events, and early access offers.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Reporting Security Issues
Security issues and bugs should be reported privately, via email, to the
Microsoft Security Response Center (MSRC) at <[secure@microsoft.com](mailto:secure@microsoft.com)>.
You should receive a response within 24 hours. If for some reason you do not, please follow up via
email to ensure we received your original message. Further information, including the
[MSRC PGP](https://technet.microsoft.com/en-us/security/dn606155) key, can be found in the
[Security TechCenter](https://technet.microsoft.com/en-us/security/default).

## License and Microsoft Support for Azure Kinect Sensor SDK

[MIT License](../../../LICENSE)

[Microsoft Support for Azure Kinect Sensor SDK](../../../microsoft-support.md)