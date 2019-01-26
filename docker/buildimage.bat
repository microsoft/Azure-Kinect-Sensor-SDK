copy ..\scripts\bootstrap-ubuntu.sh .\
docker build -t kinect_build .
del .\bootstrap-ubuntu.sh
