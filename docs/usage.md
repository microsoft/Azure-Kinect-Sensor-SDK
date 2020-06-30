# Using Azure Kinect SDK

## Installation

If you aren't making changes to the SDK itself, you should use a binary
distribution. On Windows, binaries are currently available as an MSI and a
NuGet package. On Linux, binaries are currently available as debian packages.
See below for installation instructions for each distribution mechanism. If
you would like a package that is not available please file an
[issue](https://github.com/microsoft/Azure-Kinect-Sensor-SDK/issues/new?assignees=&labels=Enhancement&template=feature-request--enhancement-.md&title=).

If you are making changes to the SDK, you can build your own copy of the SDK
from source. Follow the instruction in [building](building.md) for how to
build from source.

### MSIs

The latest stable binaries are available for download as MSIs.

   Tag                                                                               | MSI                                                                                                                                              | Firmware
-------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------
  [v1.4.1](https://github.com/microsoft/Azure-Kinect-Sensor-SDK/releases/tag/v1.4.1) | [Azure Kinect SDK 1.4.1.exe](https://download.microsoft.com/download/3/d/6/3d6d9e99-a251-4cf3-8c6a-8e108e960b4b/Azure%20Kinect%20SDK%201.4.1.exe) | [1.6.110079014](https://download.microsoft.com/download/3/d/6/3d6d9e99-a251-4cf3-8c6a-8e108e960b4b/AzureKinectDK_Fw_1.6.110079014.bin)
  [v1.4.0](https://github.com/microsoft/Azure-Kinect-Sensor-SDK/releases/tag/v1.4.0) | [Azure Kinect SDK 1.4.0.exe](https://download.microsoft.com/download/4/5/a/45aa3917-45bf-4f24-b934-5cff74df73e1/Azure%20Kinect%20SDK%201.4.0.exe) | [1.6.108079014](https://download.microsoft.com/download/4/5/a/45aa3917-45bf-4f24-b934-5cff74df73e1/Firmware/AzureKinectDK_Fw_1.6.108079014.bin)
  [v1.3.0](https://github.com/microsoft/Azure-Kinect-Sensor-SDK/releases/tag/v1.3.0) | [Azure Kinect SDK 1.3.0.exe](https://download.microsoft.com/download/e/6/6/e66482b2-b6c1-4e34-bfee-95294163fc40/Azure%20Kinect%20SDK%201.3.0.exe) | [1.6.102075014](https://download.microsoft.com/download/1/9/8/198048e8-63f2-45c6-8f96-1fd541d1b4bc/AzureKinectDK_Fw_1.6.102075014.bin)
  [v1.2.0](https://github.com/microsoft/Azure-Kinect-Sensor-SDK/releases/tag/v1.2.0) | [Azure Kinect SDK 1.2.0.msi](http://download.microsoft.com/download/1/9/8/198048e8-63f2-45c6-8f96-1fd541d1b4bc/Azure%20Kinect%20SDK%201.2.0.msi) | [1.6.102075014](https://download.microsoft.com/download/1/9/8/198048e8-63f2-45c6-8f96-1fd541d1b4bc/AzureKinectDK_Fw_1.6.102075014.bin)
  [v1.1.1](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/releases/tag/v1.1.1) | [Azure Kinect SDK 1.1.1.msi](http://download.microsoft.com/download/4/9/0/490A8EB2-FFCA-4BAD-B0AD-0581CCE438FC/Azure%20Kinect%20SDK%201.1.1.msi) | [1.6.987014](https://download.microsoft.com/download/4/9/0/490A8EB2-FFCA-4BAD-B0AD-0581CCE438FC/AzureKinectDK_Fw_1.6.987014.bin)
  [v1.1.0](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/releases/tag/v1.1.0) | [Azure Kinect SDK 1.1.0.msi](http://download.microsoft.com/download/E/B/D/EBDBB3C1-ED3F-4236-96D6-2BCB352F3710/Azure%20Kinect%20SDK%201.1.0.msi) | [1.6.987014](https://download.microsoft.com/download/4/9/0/490A8EB2-FFCA-4BAD-B0AD-0581CCE438FC/AzureKinectDK_Fw_1.6.987014.bin)
  [v1.0.2](https://github.com/Microsoft/Azure-Kinect-Sensor-SDK/releases/tag/v1.0.2) | [Azure Kinect SDK 1.0.2.msi](http://download.microsoft.com/download/B/4/D/B4D26442-DDA5-40C2-9913-3B23AE84A806/Azure%20Kinect%20SDK%201.0.2.msi) | [1.6.987014](https://download.microsoft.com/download/4/9/0/490A8EB2-FFCA-4BAD-B0AD-0581CCE438FC/AzureKinectDK_Fw_1.6.987014.bin)

The installer will put all the needed headers, binaries, and tools in the
location you choose (by default this is `C:\Program Files\Azure Kinect SDK
version\sdk`).

### NuGet

Directly include the Sensor SDK in your project using
[Nuget](https://www.nuget.org/packages/microsoft.azure.kinect.sensor/).

### Debian Package

We currently have debian packages available for Ubuntu 18.04. If you have
need for a different debian distribution, please file an
[enhancement request](https://aka.ms/azurekinectfeedback).

Our packages are hosted in [Microsoft's Package
Repository](https://packages.microsoft.com). 
* **AMD64** users, please follow [these
instructions](https://docs.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software)
to configure Microsoft's Package Repository on your machine.
* **ARM64** users, please use the same instructions, but use https://packages.microsoft.com/ubuntu/18.04/multiarch/prod for the repository path instead of the default ~~https://packages.microsoft.com/ubuntu/18.04/prod~~.


Once you have configured Microsoft's Package Repository you should have access
to the following packages:

* libk4a\<major\>.\<minor\> (Runtime package)
* libk4a\<major\>.\<minor\>-dev (Development package)
* k4a-tools (Tools package)

Please note that "\<major\>" and "\<minor\>" refer to the major and minor
portion of the version of the SDK you would like to target. For example, at the writing of these instructions the following packages are available.

* libk4a1.3
* libk4a1.3-dev
* k4a-tools

Each package contains different elements.

* Runtime package - contains shared objects needed to run executables that depend on libk4a.
* Development package - contains headers and cmake files to build against libk4a.
* Tools package - contains k4aviewer and k4arecorder

## Using tools

The installer comes with a pre-built viewer application (k4aviewer.exe) which can be used to verify the
functionality of your device and explore its capabilities. The installer puts a link to this in your Start
menu as Azure Kinect Viewer. Other command line tools; such as the recorder and firmware update utilities, are
available in the installer 'tools' directory.

## Including the SDK in your project

If you are including the Azure Kinect SDK in a C or C++ project, update your project to link to **k4a.lib** and
add the include path such that you can `#include <k4a/k4a.h>`. You also need to ensure that **k4a.dll** and **depthengine_2_0.dll** are in your path or in the same directory as your application.

For recording and playback you will need to also reference **k4arecord.lib** and the headers in include/k4arecord and have
**k4arecord.dll** in your path.

## Dependencies

The following dependencies are needed for the Azure Kinect SDK to run.

### Cross Platform Dependencies

The Azure Kinect SDK uses a closed source depth engine to interpret depth frames
coming from the depth camera. This depth engine must be in your OS's loader's
path. The depth engine is a shared object and can be found with any version
of the shipping SDK. If you are a developer, you will need to copy this
depth engine from where the SDK is installed to a location where your loader
can find it.

### Windows Dependencies

* [Depth Engine](depthengine.md)

### Linux Dependencies

* OpenSSL

* OpenGL

* [Depth Engine](depthengine.md)

## Device Setup

### Windows Device Setup

On Windows, once attached, the device should automatically enumerate and load
all drivers.

### Linux Device Setup

On Linux, once attached, the device should automatically enumerate and load
all drivers. However, in order to use the Azure Kinect SDK with the device and without
being 'root', you will need to setup udev rules. We have these rules checked
into this repo under 'scripts/99-k4a.rules'. To do so:

* Copy 'scripts/99-k4a.rules' into '/etc/udev/rules.d/'.
* Detach and reattach Azure Kinect devices if attached during this process.

Once complete, the Azure Kinect camera is available without being 'root'.

## API Documentation

See https://microsoft.github.io/Azure-Kinect-Sensor-SDK/ for the most recent API documentation, including documentation for the current
development branch.
