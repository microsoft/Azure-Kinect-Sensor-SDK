# Versioning

This describes the versioning scheme used for the Azure SDK and firmware.

## Azure Kinect SDK Versioning

Azure Kinect uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

* Increasing the major version indicates a breaking change has been made and a loss of functionality may result. The client application may require updates to use the new version.
* Increasing the minor version indicates new features have been added in a backward compatible way.
* Increasing the patch version (sometimes called iteration version) implies changes have been made to the binary in a backward compatible way.

More details about release versioning can be found [here](releasing.md)

## Firmware Versioning

The Azure Kinect firmware is composed of 4 different firmware versions. These firmware versions are exposed through 
`k4a_hardware_version_t`. Here is a sample of that hardware version:

```
RGB Sensor Version:  1.6.98
Depth Sensor Version:1.6.70
Mic Array Version:   1.6.14
Sensor Config:       5006.27
```

This version can also be simplified as `1.6.098070014` where `098`, `070`,
and `014` are the patch versions of each component version, converted to a 
zero-based 3 digit form, and concatenated.
