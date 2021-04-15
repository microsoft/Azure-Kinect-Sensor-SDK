# Script to automate creation of the Python wheel for the k4a library.

# Check that the k4a.dll and depth_engine.dll have been copied into the _libs/ folder.
# Because these dlls may not be located in a standard place, leave it to the developer
# to manually copy them into the _libs/ folder.
If (-not (Test-Path -Path "$PSScriptRoot\src\k4a\_libs\k4a.dll")) {
    Write-Host "File not found: $PSScriptRoot\src\k4a\_libs\k4a.dll"
    Write-Host "Please manually copy the k4a library into that folder."
    exit 1
}

If (-not(Test-Path -Path "$PSScriptRoot\src\k4a\_libs\depthengine*.dll")) {
    Write-Host "File not found: $PSScriptRoot\src\k4a\_libs\depthengine*.dll"
    Write-Host "Please manually copy the depth engine library into that folder."
    exit 1
}

# Create a virtual environment and activate it.
Write-Host "Creating a Python virtual environment."

If (Test-Path -Path "temp_build_venv") {
    Remove-Item -LiteralPath "temp_build_venv" -Force -Recurse
}

If (Test-Path -Path "build") {
    Remove-Item -LiteralPath "build" -Force -Recurse
}

python -m venv temp_build_venv
./temp_build_venv/Scripts/activate

# Install the package in editable mode so that it installs dependencies. These are needed for sphinx docs.
python -m pip install --upgrade pip
pip install -e .

# Build the .whl file and place it in a build/ folder.
pip install wheel
pip wheel . -w build
Remove-Item ./build/* -Exclude k4a*.whl -Recurse -Force


# Build the docs and move them to the build/ folder. doxygen must be in the path.
if (Get-Command doxygen -errorAction SilentlyContinue) {
    mkdir ./build/docs
    doxygen ./Doxyfile

    # Create a convenient documentation.html that redirects to the index.html that doxygen generates.
    '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "https://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
    <html xmlns="http://www.w3.org/1999/xhtml">
    <head>
    <meta http-equiv="Content-Type" content="text/xhtml;charset=UTF-8"/>
    <meta http-equiv="X-UA-Compatible" content="IE=9"/>
    <meta http-equiv="REFRESH" content="0;URL=html/index.html">
    </html>' > ./build/docs/documentation.html
}
else {
    Write-Host "doxygen not found, skipping building the documentation."
}


# Deactivate virtual environment and delete it.
./temp_build_venv/Scripts/deactivate.bat
Remove-Item -LiteralPath "temp_build_venv" -Force -Recurse

Write-Host "Done creating k4a package wheel."