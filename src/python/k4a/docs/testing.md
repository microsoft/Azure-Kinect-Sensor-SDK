# Testing Azure Kinect Python API(K4A)

The tests/ folder contains Python tests to check for the quality of the
Python K4A library. The tests should be run at least on every code submission
into the main branch of the repository.

## Test Prerequisites

The following are required in order to run the Python K4A tests.

1. Python (>= 3.6)

2. An internet connection to download required Python packages.

3. The depth engine and k4a libraries are placed in src/python/k4a/src/k4a/_libs.
   See building.md for more info on these binaries.

4. An attached Azure Kinect device.

## Running tests

>**Note:** The commands `python` and `pip` in Windows are assumed to be `python3` and `pip3`.
> In Linux, they are not aliased to Python3, so `python3` and `pip3` may have to be used in
> the instructions below.

### Setting up the test environment

1. Copy the k4a and depth engine dynamic libraries (.dll or .so) into the folder 
   src/python/k4a/src/k4a/_libs. 

   >**Note:** Read [building.md](./building.md) for more information on how to copy the libraries into
   >the _libs folder. The file names are slightly different in Windows and Linux.  

2. In a command line terminal, create a Python virtual environment and activate it (do not include brackets):  
    `cd <repo_root>/src/python/k4a`  
    `python -m venv <env_name>`  
      
    Windows:  
    `.\<env_name>\Scripts\activate`  
      
    Ubuntu:  
    `source ./<env_name>/bin/activate`  
      
3. Install the k4a package in development mode. This is required in order to
   automatically install required Python packages for running the tests, as
   well as to install the code with the k4a package as the root of the 
   subpackages and modules. In the command below, make sure to include 
   ".[test]" with dot and square brackets.  
    `pip install -e .[test]`

### Running unit tests on the command line

Unit tests execute small pieces of code that run very fast (less than a second).
These tests do not require any hardware to be attached.
These tests should be run on every pull request.
      
To run the unit tests:  
    `python -m pytest tests -k _unit_`
      
To capture the results, use an additional --junit-xml=./test_results.xml option.

### Running functional tests on the command line

Functional tests execute code that exercises the functionality of an attached device.
There are 2 types of functional tests:
    1. functional_fast - fast functional tests that should be run on every pull request.
    2. functional - more comprehensive functional tests but these may take a long time.
      
To run the functional_fast tests:  
    `python -m pytest tests -k functional_fast`
      
To run all functional tests:  
    `python -m pytest tests -k functional`
      
To capture the results, use an additional --junit-xml=./test_results.xml option.

### Running performance tests on the command line

Performance tests attempt to verify the performance of the device under stress,
such as maintaining frame rates, etc. These tests require an attached device.
These tests should be run on every pull request.

>**Note:** There are currently no performance tests. This section is here as a placeholder
>for planned performance tests in the future.

To run the performance tests:   
    `python -m pytest tests -k perf`
      
To capture the results, use an additional --junit-xml=./test_results.xml option.

### Running tests on an IDE

The tests can also be run on an IDE such as Visual Studio or Visual Studio Code.
The specific instructions for doing so will not be detailed here.
In general, the steps are:

Run steps 1-3 above before starting the IDE. 

In Visual Studio Code, Ctrl+Shift+P will let you select the Python interpreter (virtual environment). You may also need to configure the Python settings to use pytest
framework if it is not turned on by default.
https://code.visualstudio.com/docs/python/environments

In Visual Studio, there is a Python environment selector.
https://docs.microsoft.com/en-us/visualstudio/python/selecting-a-python-environment-for-a-project?view=vs-2019

## Expected Failures and Skipped Tests

There are some tests that are decorated with "@unittest.skip" or "@unittest.expectedfail".
These are tests that for some reason always fail. They affect a small subset of the Python K4A API.
They are annonated as such so that automated test pipelines will not get hung up.
The annotations will be removed whenever the tests are fixed to be passing.

When running the Test_K4A_AzureKinect tests, these may take a
very long time. Consider running these tests one by one.

When running the Test_Transformation_AzureKinect tests, this may take a
very long time. Consider running these tests one by one. Additionally, the tests
capture live data and may fail if the data cannot be transformed from one camera
to another. For example, this may happen if the selected pixel to transform is an
invalid pixel in the depth image just because the data at that pixel is not in the
range of depths that the depth sensor can see.
