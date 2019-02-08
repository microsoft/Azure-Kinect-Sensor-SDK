# Using K4A SDK

## Dependencies

The K4A SDK depends on a depth engine plugin to interact with the depth
camera. This plugin comes as part of the SDK. On Linux it is named
libdepthengine.so. On Windows it is named depthengine_major_minor.dll where
"major" and "minor" are the major and minor number.

You can download this plugin from the depth engine. Microsoft internal
employees can also consume this plugin by using Universal Packages. To do so,
please follow the steps
[here](https://docs.microsoft.com/en-us/azure/devops/artifacts/quickstarts/universal-packages?view=azure-devops&tabs=azuredevops#prerequisites)
to install the Azure CLI and login to Azure DevOps. If these tools are
installed, CMake will grab the correct version of the depthengine plugin as
part of the build.

The following dependencies are needed for the K4A SDK to run on different
platforms.

### Windows Dependencies

No dependencies needed. The K4A SDK is self contained on Windows and contains
all needed dependencies.

### Linux Dependencies

* OpenSSL

* LibUSB

* OpenGL

## Device Setup

### Windows Device Setup

On Windows, once attached, the device should automatically enumerate and load
all drivers.

### Linux Device Setuo

On Linux, once attached, the device should automaticaly enumerate and load
all drivers. However, in order to use the K4A SDK with the device and without
being root, you will need to setup udev rules. We have these rules checked
into this repo under scripts/99-k4a.rules. If you copy that file into
/etc/udev/rules.d/ you will be able to use the K4A SDK, on the next device
enumeration without being root. Therefore, if the device was already plugged in,
unplug it ang plug it back in.