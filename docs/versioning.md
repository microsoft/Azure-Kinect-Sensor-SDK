# Versioning

This describes the versioning scheme used for the Azure SDK and Firmware.

## Azure Kinect SDK Versioning

Azure Kinect uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

* Increasing the major version indicates a breaking change has been made and a loss of functionality may result.
* Increasing the minor version indicates new features have been added in a backwards compatible way.
* Increasing the patch version (sometimes called iteration version) implies new changes have been made to the binary in a backwards compatible way.

## FW Versioning

The Azure Kinect firmware is composed of 4 different firmware versions. These firmware versions are exposed through 
```k4a_hardware_version_t```. Here is a sample of that hardware version.

```shell
RGB Sensor Version:  1.6.98
Depth Sensor Version:1.6.70
Mic Array Version:   1.6.14
Sensor Config:       5006.27
```

This version can also be simplified as ```1.6.987014``` where ```98```, ```70```, and ```14``` are the concatenated 
Patch values from each of the RGB, Depth, and Mic array Semantic versions.