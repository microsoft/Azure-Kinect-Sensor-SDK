# Azure Kinect SDK (K4A)

Welcome to the Azure Kinect DK Sensor SDK! While devices and the Body Tracking SDK will be available this
summer, we wanted to share the Sensor SDK now. We hope you can use this interim period to get familiar with
our SDK, ask questions, and provide feedback. See [Azure.com/Kinect](https://Azure.com/kinect) for device
info and available documentation. 

[![Build Status (develop)](https://dev.azure.com/ms/Azure-Kinect-Sensor-SDK/_apis/build/status/Microsoft.Azure-Kinect-Sensor-SDK%20(Public)?branchName=develop)](https://dev.azure.com/ms/Azure-Kinect-Sensor-SDK/_build/latest?definitionId=133&branchName=develop)

## Introduction

**Azure Kinect SDK** is a cross platform (Linux and Windows) user mode SDK to read data from your Azure Kinect device.

## Why use the Azure Kinect SDK 

The Azure Kinect SDK enables you to get the most out of your Azure Kinect camera. Features include:

* Depth camera access
* RGB camera access and control (e.g. exposure and white balance)
* Motion sensor (gyroscope and accelerometer) access
* Synchronized Depth-RGB camera streaming with configurable delay between cameras
* External device synchronization control with configurable delay offset between devices
* Camera frame meta-data access for image resolution, timestamp and temperature
* Device calibration data access

## Installation

To use the SDK please refer to the installation instructions here.

## Documentation and Official Builds

The latest stable binaries are available for download.
   
   Tag   | Download
---------|----------------
  [v1.0.2](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/releases/tag/v1.0.2) | [Azure Kinect SDK 1.0.2.msi](http://download.microsoft.com/download/B/4/D/B4D26442-DDA5-40C2-9913-3B23AE84A806/Azure%20Kinect%20SDK%201.0.2.msi)

Please see [usage](docs/usage.md) for info on how to use the SDK.

## Building

Azure Kinect SDK uses CMake to build. For instructions on how to build this SDK please see
[building](docs/building.md).

## Versioning

The Azure Kinect SDK uses semantic versioning, please see [versioning.md](docs/versioning.md) for more information.

## Testing

For information on writing or running tests, please see [testing.md](docs/testing.md)

## Contribute

We welcome your contributions! Please see the [contribution guidelines](CONTRIBUTING.md).

## Feedback

For SDK feedback or to report a bug, please file a [GitHub Issue](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/issues). For general suggestions or ideas, visit our [feedback forum](https://aka.ms/azurekinectfeedback).

## Join Our Developer Progarm

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

## License

[MIT License](LICENSE)

