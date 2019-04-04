# K4A SDK

[![Build Status (develop)](https://microsoft.visualstudio.com/Analog/_apis/build/status/Analog/AI/depthcamera/Microsoft.Azure-Kinect-Sensor-SDK?branchName=develop)](https://microsoft.visualstudio.com/Analog/_build/latest?definitionId=35486&branchName=develop)

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Introduction

**Azure Kinect Sensor SDK** is a cross platform (Linux and Windows) user mode
SDK to read data from your Azure Kinect device.

### Why use the Azure Kinect Sensor SDK 

The Azure Kinect Sensor SDK enables you to get the most out of your Azure
Kinect camera. Features include:
* Depth camera access
* RGB camera access and control (e.g. exposure and white balance)
* Motion sensor (gyroscope and accelerometer) access
* Synchronized Depth-RGB camera streaming with configurable delay between
  cameras
* External device synchronization control with configurable delay offset
  between devices
* Camera frame meta-data access for image resolution, timestamp and temperature
* Device calibration data access

## Getting Started

### Installation

To use the SDK please refer to the installation instructions here.

## Documentation and Official Builds

Please see [usage](docs/usage.md) for info on how to use the SDK.

## Building

Azure Kinect Sensor SDK uses cmake to build. For instructions on how to build
this SDK please see [building](docs/building.md).

## Versioning

The Azure Kinect Sensor SDK uses semantic versioning. 

## Testing

K4A has five types of tests:
* Unit tests
* Functional tests
* Stress tests
* Performance tests
* Firmware tests

For more instructions on running and writing tests see
[testing](docs/testing.md).

## Contribute
We welcome your contributions! Please see the [contribution
guidelines](docs/contributing.md).

### Feedback
For any feedback or to report a bug, please file a [GitHub
Issue](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/issues).

### Code of Conduct
This project has adopted the [Microsoft Open Source Code of
Conduct](https://opensource.microsoft.com/codeofconduct/).  For more
information see the [Code of Conduct
FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional
questions or comments.

## License
[MIT License](LICENSE)
