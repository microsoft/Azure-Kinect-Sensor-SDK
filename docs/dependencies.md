# External Dependencies

The Azure Kinect repo consumes external dependencies through compiling from source.
It is preferred to build from source when possible due to the lack of
standard package management on Windows.

## Building from Source

To build from source, the Azure Kinect repo must first download the source.

The Azure Kinect repo uses git submodules to download the source. These submodules are
initialized on the first CMake configure by executing

```
git submodule update --init --recursive
```

CMake will also set the submodules.recurse configuration so that all git
commands which can recurse to submodules will in the future. Users can turn
this off if they would like but do so at their own risk.

Previously, CMake's FetchContent module was used to download external
dependencies but there was a large performance hit on reconfiguration which
forced the usage of git submodules.

All the submodules are cloned into ./extern/project_name/src. A
CMakeLists.txt exists in ./extern/project_name which contains the logic
to add the project as a target to the Azure Kinect build graph.

## Linking against pre-built binaries

In order to link against pre-built binaries, those binaries must exist on
disk. Use of a package manager is expected for the typical case of getting
these binaries. On Windows, CMake's FetchContent can be used.

Once the binaries are downloaded it is up to the programmer to properly
generate a CMake target for the library (if the package doesn't include one
already)
