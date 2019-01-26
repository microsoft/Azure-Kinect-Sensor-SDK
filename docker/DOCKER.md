# Building with Docker

This Dockerfile defines a Linux build environment for Kinect for Azure.

Visual Studio 2017 on Windows can connect to this Docker container to cross
compile for Linux.

To create the Docker image run buildimage.bat. This will be slow the first time
you run it as it sets up the image.

Run runimage.bat to start the Docker container. The script will tell you the
port number and credentials to connect to using SSH.

In Visual Studio under Tools / Options / Cross Platform add a connection to your
Docker container on localhost.

To clean up the running image and delete all of its storage run cleanupimage.cmd
