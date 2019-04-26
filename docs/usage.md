# Using Azure Kinect SDK

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

No dependencies needed. The Azure Kinect SDK is self contained on Windows and contains
all needed dependencies.

### Linux Dependencies

* OpenSSL

* LibUSB

* OpenGL

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
* Dettach and reattch Azure Kinect devices if attached durring this process.

Once complete, the Azure Kinect camera is available without being root.