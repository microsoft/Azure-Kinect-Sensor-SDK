#!/bin/bash

# Script to automate creation of the Python wheel for the k4a library.
# In Ubuntu, the command for the base installation of Python 3 is python3.
# Also, pip and venv may need to be installed manually before running this
# script since it may not come with the base installation of Python 3.
# To get pip: sudo apt install python3-pip
# To get venv: sudo apt install python3-venv
# To get doxygen: sudo apt install doxygen

# Get the directory where this script lives.
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

# Check that the k4a.so and depth_engine.so have been copied into the _libs/ folder.
# Because these dlls may not be located in a standard place, leave it to the developer
# to manually copy them into the _libs/ folder.
if [ ! -f "$DIR/src/k4a/_libs/libk4a.so" ] && [ ! -h "$DIR/src/k4a/_libs/libk4a.so" ]; then
    echo "File not found: $DIR/src/k4a/_libs/libk4a.so"
    echo "Please manually copy the k4a library into that folder."
    return 1
fi

if [ ! -f "$DIR/src/k4a/_libs/libdepthengine"* ] && [ ! -h "$DIR/src/k4a/_libs/libdepthengine"* ]; then
    echo "File not found: $DIR/src/k4a/_libs/libdepthengine*"
    echo "Please manually copy the depth engine library into that folder."
    return 1
fi

# Create a virtual environment and activate it.
echo "Creating a Python virtual environment."

if [ -d "temp_build_venv" ]; then
    rm -rf "temp_build_venv"
fi

if [ -d "build" ]; then
    rm -rf "build"
fi

python3 -m venv temp_build_venv
source ./temp_build_venv/bin/activate

# Install the package in editable mode so that it installs dependencies. These are needed for sphinx docs.
python -m pip install --upgrade pip
pip install -e .

# Build the .whl file and place it in a build/ folder.
pip install wheel
pip wheel . -w build

# Build the docs and move them to the build/ folder. doxygen must be in the path.
if command -v doxygen &> /dev/null
then
    mkdir ./build/docs
    doxygen ./Doxyfile
else
    echo "doxygen not found, skipping building the documentation."
fi

# Create a convenient documentation.html that redirects to the index.html that doxygen generates.
echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "https://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/xhtml;charset=UTF-8"/>
<meta http-equiv="X-UA-Compatible" content="IE=9"/>
<meta http-equiv="REFRESH" content="0;URL=html/index.html">
</html>' > ./build/docs/documentation.html


# Deactivate virtual environment and delete it.
deactivate
rm -rf "temp_build_venv"

echo "Done creating k4a package wheel."
