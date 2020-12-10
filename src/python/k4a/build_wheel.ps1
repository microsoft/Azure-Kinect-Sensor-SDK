# Script to automate creation of the Python wheel for the k4a library.

# Check that the k4a.dll and depth_engine_2_0.dll have been copied into the _libs/ folder.
# Because these dlls may not be located in a standard place, leave it to the developer
# to manually copy them into the _libs/ folder.
If (-not (Test-Path -Path "$PSScriptRoot\src\k4a\_libs\k4a.dll")) {
    Write-Host "File not found: $PSScriptRoot\src\k4a\_libs\k4a.dll"
    Write-Host "Please manually copy the dll into that folder."
    exit 1
}

If (-not(Test-Path -Path "$PSScriptRoot\src\k4a\_libs\depthengine*.dll")) {
    Write-Host "File not found: $PSScriptRoot\src\k4a\_libs\depthengine*.dll"
    Write-Host "Please manually copy the dll into that folder."
    exit 1
}

# Create a virtual environment and activate it.
Write-Host "Creating a Python virtual environment."

If (Test-Path -Path "temp_build_venv") {
    Remove-Item -LiteralPath "temp_build_venv" -Force -Recurse
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


# Build the docs and move them to the build/ folder.
#cd ./docs
#powershell -File ./build_docs.ps1
#Move-Item ./build/html ../build/docs/


# Deactivate virtual environment and delete it.
./temp_build_venv/Scripts/deactivate.bat
Remove-Item -LiteralPath "temp_build_venv" -Force -Recurse


# Copy the docs/ folder into the build/ folder.
#Copy-Item -Path .\docs -Destination .\build -Recurse