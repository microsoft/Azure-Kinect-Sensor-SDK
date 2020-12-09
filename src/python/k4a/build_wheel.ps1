# Create a virtual environment and activate it.
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