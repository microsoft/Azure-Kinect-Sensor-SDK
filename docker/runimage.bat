:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

@echo off
docker stop k4a > NUL 2>&1
docker rm k4a > NUL 2>&1

ECHO Creating volumes
docker volume create k4a-src
docker volume create k4a-build
ECHO Running k4a build environment
docker run -i -d -p 2022:22 --name k4a -v k4a-src:/var/tmp/src -v k4a-build:/var/tmp/build --security-opt seccomp:unconfined kinect_build
ECHO Connect via SSH to the following port 
ECHO NOTE: Replace 0.0.0.0 with one of the host machine's IP address and use port 2022
docker port k4a 22
ECHO Username 'k4a' password 'kinect'

