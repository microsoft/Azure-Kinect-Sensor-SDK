# Using K4A SDK

## Dependencies

The following dependencies are needed for the K4A SDK to run.

### Cross Platform Dependencies

The K4A SDK uses a closed source depth engine to interpret depth frames
coming from the depth camera. This depth engine must be in your OS's loader's
path. The depth engine is a shared object and can be found with any version
of the shipping SDK. If you are a developer, you will need to copy this
DepthEngine from where the SDK is installed to a location where your loader
can find it.

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