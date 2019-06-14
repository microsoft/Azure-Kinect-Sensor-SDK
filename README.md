# Azure Kinect SDK (K4A)

Welcome to the Azure Kinect DK Sensor SDK! While devices and the Body Tracking SDK will be available this
summer, we wanted to share the Sensor SDK now. We hope you can use this interim period to get familiar with
our SDK, ask questions, and provide feedback. See [Azure.com/Kinect](https://Azure.com/kinect) for device
info and available documentation.

[![Build Status
(develop)](https://dev.azure.com/ms/Azure-Kinect-Sensor-SDK/_apis/build/status/Microsoft.Azure-Kinect-Sensor-SDK%20(Public)?branchName=develop)](https://dev.azure.com/ms/Azure-Kinect-Sensor-SDK/_build/latest?definitionId=133&branchName=develop)
[![Nuget](https://img.shields.io/nuget/vpre/Microsoft.Azure.Kinect.Sensor.svg)](https://www.nuget.org/packages/Microsoft.Azure.Kinect.Sensor/)

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

To use the SDK, please refer to the installation instructions in [usage](docs/usage.md)

## Documentation

API documentation is avaliable [here](https://microsoft.github.io/Azure-Kinect-Sensor-SDK/).

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

## Q&A

### What does it mean that it is DK? DK stands for Developer Kit and it is a combination of 4 different sensors?

 (1MP Time-of-Flight, depth camera, 7-microphone array, 12MP RGB camera, and IMU). This device is meant for developers not consumers. It is meant for use in an ambient temperature range of 10-25 degrees Celsius. Azure Kinect DK is not a replacement for Xbox Kinect.

### Can I only use Azure Kinect DK with Azure?

No. Azure Kinect DK was designed to make it easy for developers to work with Azure, but you can use it with or without any cloud provider.

### What types of edge processing support is the Azure Kinect sensor capable of?

Azure Kinect does not have onboard compute so you can pair it with your choice of PC from screenless to desktop workstation. The system requirements are Windows® 10 PC or Ubuntu 18.04 LTS with 7th Generation Intel® CoreTM i3 Processor (Dual Core 2.4 GHz with HD620 GPU or faster), USB 3.0 Port, 4 GB RAM. Not available on Windows 10 in S mode. Skeletal/body tracking and other experiences may require more advanced PC hardware.

### What are the bandwidth requirements for Azure Kinect cloud connection? 

It depends on your use case and the service that you are running. Check out aka.ms/Kinectdocs for more info.

### Can I detach any of the sensors from the device?

No sensors are detachable from the device.

## Can more than one sensor be connected to a single PC?

Yes, performance depends use case and PC configuration.

### What is the maximum number of devices in a Azure Kinect sync chain?

Your limit will depend on your use case but we have tested up to 10.

### Is there a limit to the number of cameras that can be used in a single location with overlapping fields of view?

There is no limit to the number of cameras in a single location.

### Can multiple cameras be synchronised to achieve higher frame rates?

There is no theoretical reason why this wouldn’t be possible. However, this exact scenario has not been tested.

### Will Azure Kinect ship with calibration software that enables a multi-sensor configuration to determine the relative positions and orientations of the connected cameras?

No calibration software is included for multi-sensor set-ups.

### Will body tracking work across multiple sensors?

Body tracking works with a single sensor.

### Is there support for UWP, C++/C#, Unity, and Python?

The Speech SDK supports multiple languages, for details see https://docs.microsoft.com/en-us/azure/cognitive-services/speech-service/.The Sensor SDK supports low level C API. Additional language wrappers starting with C++ and C# will be rolled out in the future.

### Will the Azure Kinect also support the HoloLens 2 hand tracking API?

Not at this time.

### Will Microsoft supply any hand pose estimation software with the Azure Kinect?

We are planning on providing hand position and orientation (relative to wrist).

### How many people will be able the Azure Kinect to track simultaneously?

The performance of the body tracking algorithm is limited by available compute. The number of skeletons that can be tracked depends on the available compute and desired frame rate of your application. The sensor field of view imposes a practical limit of around 15 people.

### What is the outdoor performance like?

Azure Kinect DK is designed for indoor use. For complete details of operating environments, visit aka.ms/kinectdocs.

### How does it cope with a sunlit scene or shaded scene?

Azure Kinect DK is designed for indoor use. With any time of flight sensor, there is sensitivity to sunlight and other reflectivity.

### Can the directional microphone pinpoint the spatial location of an audio source in 3 dimensions?

The spatial location is currently not exposed via an API.

### Can it separate audio sources - the dinner party problem?

Not at this time with the Speech SDK. Developers can implement this based on the raw audio capture.

### Does the Azure Kinect necessarily need a connected PC or in the future you will release something to connect it automatically to the cloud?

Azure Kinect does not have onboard compute so you can pair it with your choice of PC from screenless to desktop workstation, either of which can be cloud connected. We look forward to seeing what developers build, listening, and determining our roadmap accordingly.

### Will the Azure Kinect support higher frame rates in the future?

We are always striving to improve our sensor technology. We look forward to learning more about the solutions our customers want to create to inform our roadmap of sensor capabilities.

### When will it be available in markets besides US and China?

 We hope to announce additional markets later this year.

### Is there an Android, iOS or macOS SDK?

 Right now we're focused on Windows and Linux, but the Sensor SDK will be open source enabling contributions to this effort if desired by the community.

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

## License

[MIT License](LICENSE)

