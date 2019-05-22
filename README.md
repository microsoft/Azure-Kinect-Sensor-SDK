# Azure-Kinect-Sensor-SDK API Documentation

This branch contains API documentation generated from doxygen comments in the SDK API headers.

The documentation is visible on the [Azure-Kinect-Sensor-SDK github.io page](https://microsoft.github.io/Azure-Kinect-Sensor-SDK/).

## Publishing

Publishing is automatically performed as part of builds of official branches in the repo.
Updates are only published if there are changes to the documentation output.

## Previewing documentation changes

To generate a local copy of the documentation, configure the cmake build with `-DK4A_BUILD_DOCS` and build the `k4adocs` target. 

You can preview the documentation in the build output under `docs\html\index.html`.

## Branches

The automatic documentation publishing will update the most recent documentation for each of the branches in the repo. Deleted
branches need to periodically have their documentation pages deleted manually.
