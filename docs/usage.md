# Using Azure Kinect SDK

## Installation

If you aren't making changes to the SDK itself, you should use the binary distribution. On Windows
this is currently available as an installer. The binaries will soon be available in NuGet and Debian
packages as well.

Links to the latest downloads are on the main [README.md](../README.md#documentation-and-official-builds).

The installer will put all the needed headers, binaries, and tools in the location you choose (by default this
is `C:\Program Files\Azure Kinect SDK version\sdk`).

If you want to build your own copy of the SDK, you can follow the instruction in [building](building.md) for how to build
from source.

## Using tools

The installer comes with a pre-built viewer application (k4aviewer.exe) which can be used to verify the
functionality of your device and explore its capabilities. The installer puts a link to this in your start
menu as Azure Kinect Viewer. Other command line tools, such as the recorder and firmware update utilites, are
available in the installer tools directory.

## Including the SDK in your project

If you are including the Azure Kinect SDK in a C or C++ project, update your project to link to **k4a.lib** and 
add the incude path such that you can `#include <k4a/k4a.h>`. You also need to ensure that **k4a.dll** and **depthengine_1_0.dll** are in your path or in the same directory as your application.

For recording and playback you will need to also reference **k4arecord.lib** and the headers in include/k4arecord, and have
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
being root, you will need to setup udev rules. We have these rules checked
into this repo under scripts/99-k4a.rules. To do so:

* Copy scripts/99-k4a.rules into /etc/udev/rules.d/.
* Detach and reattach Azure Kinect devices if attached during this process.

Once complete, the Azure Kinect camera is available without being root.

## API Documentation

See https://microsoft.github.io/Azure-Kinect-Sensor-SDK/ for the most recent API documentation, including documentation for the current
development branch.
