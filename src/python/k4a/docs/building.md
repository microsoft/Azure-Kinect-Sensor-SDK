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
  for the binary. The Windows library is k4a.dll, and the Linux library is libk4a.so.
  The k4a binary needs to be copied to the host system and added 
  to the path src/python/k4a/src/k4a/_libs in this repository before building.

* [Depth Engine](../../../../docs/depthengine.md). 
  The depth engine (DE) is a closed source binary shipped with the
  SDK installer. The DE binary needs to be copied to the host system and added 
  to the path src/python/k4a/src/k4a/_libs in this repository before building.

The following dependencies are for both Windows and Linux, but in Windows they seems to be
automatically part of a Python installation, while on Linux they need to be installed
separately:

* Python module pip  
   On Linux, install pip with:  
   `sudo apt install python3-pip -y`  
   
* Python module venv  
   On Linux, install venv with:  
   `sudo apt install python3-venv -y`

The following tools are optional:

* [doxygen](https://github.com/doxygen/doxygen)
  Required for building documentation. If not installed, html documentation will not be built.


## Building

### Building using a powershell script (Windows)

1. Copy the k4a and DE binaries into the folder src/python/k4a/src/k4a/_libs. 
   The file name for the k4a library MUST be k4a.dll.
   
    >**Note:** Python will look for "k4a.dll" in Windows to load the k4a library, 
    >which will then load the depth engine dll.
    >An example of the files to put in \_libs/ for Windows are: 
    >- k4a.dll
    >- depthengine\_2\_0.dll

2. In a powershell terminal, run the script src/python/k4a/build_wheel.ps1.
   This will create the .whl file in a build/ folder.

### Building using a bash script (Linux)

1. Copy the k4a and DE binaries into the folder src/python/k4a/src/k4a/_libs.
   The file name for the k4a library MUST be libk4a.so.
   
    >**Note:** Python will look for "libk4a.so" in Linux to load the k4a library,
    >which will then load the depth engine dll.
    >An example of the files to put in \_libs/ for Linux are:  
    >- libk4a.so (link) -> libk4a.so.1.4
    >- libk4a.so.1.4 (link) -> libk4a.1.4.1
    >- libk4a.so.1.4.1
    >- depthengine.so.2.0

2. In a terminal, source the script src/python/k4a/build_wheel.csh.
   This will create the .whl file in a build/ folder.
   
    `cd <repo_root>/src/python/k4a`  
    `source build_wheel.csh`  
   
### Building using a command line terminal (cross platform)

1. Copy the k4a and DE binaries into the folder src/python/k4a/src/k4a/_libs.
   The file names for the k4a library MUST be k4a.dll (for Windows) and libk4a.so (for Linux).
   
    >**Note:** Python will look for "k4a.dll" in Windows and "libk4a.so" in Linux to
    >load the k4a library, which will then load the depth engine dll.
    >An example of the files to put in \_libs/ for cross-platform compatibility are: 
    >- libk4a.so (link) -> libk4a.so.1.4
    >- libk4a.so.1.4 (link) -> libk4a.1.4.1
    >- libk4a.so.1.4.1
    >- depthengine.so.2.0
    >- k4a.dll
    >- depthengine\_2\_0.dll

2. In a command line terminal, create a Python virtual environment and activate it (do not include brackets):
    `cd <repo_root>/src/python/k4a`  
    `python -m venv <env_name>`  
    `./<env_name>/Scripts/activate`  
      
3. Build the .whl file and place it in a build/ folder.
    `pip install wheel`  
    `pip wheel . -w build`  

4. Deactivate the virtual environment and delete it.  
    `./<env_name>/Scripts/deactivate.bat`  
    Delete the directory `<env_name>`.
      
### Building the HTML Documentation

A Doxygen settings file is provided in the project directory.

1. Run doxygen using the settings file to create the html documentation.  
    `doxygen Doxyfile`

The output files are put in the build/docs/ folder.  The main page is build/docs/html/index.html. 

The build scripts will also run doxygen as part of the build if doxygen is installed.
      
## Installing

The build process will create a k4a*.whl wheel file that can be installed via pip.
The wheel file can be distributed and installed as follows.

1. In a command line terminal, install the k4a python library via pip.
   Replace <k4a*.whl> with the name of the wheel file.
    `pip install <k4a*.whl>`
      
Once installed, the user can "import k4a" in their python code.
