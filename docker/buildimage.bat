:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

copy ..\scripts\bootstrap-ubuntu.sh .\
docker build -t kinect_build .
del .\bootstrap-ubuntu.sh
