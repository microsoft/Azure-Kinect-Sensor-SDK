# Building Azure Kinect Python API(K4A)

This information is for how to build your own copy of the Python wheel. 
If you need to build from source, you have to clone the repository to ensure all the submodule dependencies in place.

## Support Configurations

Python:
* Greater than or equal to version 3.6.

Architectures: 
* amd64
* x86 (limited, no testing is performed)

OS:
* Windows
* Linux

## Dependencies

Building on Windows and Linux requires an active internet connection. Part of the
build will require downloading dependencies.

The following dependencies are for both Windows and Linux:

* [python3](https://www.python.org/getit/). During the install make sure to add
  python to path.
  
* [k4a library](../../../../docs/building.md)
  The k4a library can be built from the repository or the SDK can be downloaded
  for the binary. The Windows library is k4a.dll, and the Linux library is k4a.so.
  The k4a binary needs to be copied to the host system and added 
  to the path src/python/k4a/src/k4a/_libs in this repository before building.

* [Depth Engine](../../../../docs/depthengine.md). 
  The depth engine (DE) is a closed source binary shipped with the
  SDK installer. The DE binary needs to be copied to the host system and added 
  to the path src/python/k4a/src/k4a/_libs in this repository before building.

The following tools are optional:

* [doxygen](https://github.com/doxygen/doxygen)
  Required for building documentation.


## Building

### Building using a powershell script (Windows)

1. Copy the k4a and DE binaries into the folder src/python/k4a/src/k4a/_libs.
   The file names MUST be k4a.dll and depthengine.dll. 
   Note: If the dlls have different names, create a symlink with relative redirection. This may
         require elevated permissions to create the symlink as well as run build_wheel.ps1.

2. In a powershell terminal, run the script src/python/k4a/build_wheel.ps1.
   This will create the .whl file in a build/ folder.

### Building using a bash script (Linux)

1. Copy the k4a and DE binaries into the folder src/python/k4a/src/k4a/_libs.
   The file names MUST be libk4a.so and libdepthengine.so.
   Note: If the .so have different names, create a symlink with relative redirection. This may
         require elevated permissions to create the symlink as well as run build_wheel.csh.

2. In a terminal, source the script src/python/k4a/build_wheel.csh.
   This will create the .whl file in a build/ folder.
   
### Building using a command line terminal (cross platform)

1. Copy the k4a and DE binaries into the folder src/python/k4a/src/k4a/_libs.
   The file names MUST be k4a.dll and depthengine.dll in Windows, and
   the file names MUST be libk4a.so and libdepthengine.so in Linux.
   
   Note: If the libraries have different names, create a symlink with relative redirection. This may
         require elevated permissions to create the symlink.

2. In a command line terminal, create a Python virtual environment and activate it (do not include brackets):
      cd <repo_root>/src/python/k4a
      python -m venv <env_name>
      ./<env_name>/Scripts/activate
      
3. Build the .whl file and place it in a build/ folder.
      pip install wheel
      pip wheel . -w build

4. Deactivate the virtual environment and delete it.
      ./<env_name>/Scripts/deactivate.bat
      Delete the directory <env_name>
      
### Building the HTML Documentation

A Doxygen settings file is provided in the project directory. Run doxygen using this
settings file to create the html documentation.
      
## Installing

The build process will create a k4a*.whl wheel file that can be installed via pip.
The wheel file can be distributed and installed as follows.

1. In a command line terminal, install the k4a python library via pip.
   Replace <k4a*.whl> with the name of the wheel file.
      pip install <k4a*.whl>
      
Once installed, the user can "import k4a" in their python code.