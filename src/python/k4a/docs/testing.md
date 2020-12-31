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

### Running tests on the command line

1. Copy the k4a and DE binaries into the folder src/python/k4a/src/k4a/_libs.

2. In a command line terminal, create a Python virtual environment and activate it (do not include brackets):
      cd <repo_root>/src/python/k4a
      python -m venv <env_name>
      ./<env_name>/Scripts/activate
      
3. Install the k4a package in development mode. This is required in order to
   automatically install required Python packages for running the tests, as
   well as to install the code with the k4a package as the root of the 
   subpackages and modules.
      pip install -e .[test]   (include "[test]" with square brackets)
      
4. Run the tests in python. To capture the results, use an additional --junit-xml=./test_results.xml option.
      python -m pytest tests

### Running tests on an IDE

The tests can also be run on an IDE such as Visual Studio or Visual Studio Code.
The specific instructions for doing so will not be detailed here.
In general, the steps are:

Run steps 1-3 above before starting the IDE. 

In Visual Studio Code, Ctrl+Shift+P will let you select the Python interpreter (virtual environment).
https://code.visualstudio.com/docs/python/environments

In Visual Studio, there is a Python environment selector.
https://docs.microsoft.com/en-us/visualstudio/python/selecting-a-python-environment-for-a-project?view=vs-2019

## Expected Failures and Skipped Tests

There are some tests that are decorated with "@unittest.skip" or "@unittest.expectedfail".
These are tests that for some reason always fail. They affect a small subset of the Python K4A API.
They are annonated as such so that automated test pipelines will not get hung up.
The annotations will be removed whenever the tests are fixed to be passing.