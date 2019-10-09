# Azure Kinect Fast Capture:streaming

## Introduction

The Azure Kinect Fast Capture streaming process keeps streaming both the color and depth frames. It will store
the most recent frames to the specified folder when notified by [Azure Kinect Fast Capture trigger](../k4afastcapture_trigger/README.md).

## Usage Info

```
d:\fastcapture_streaming.exe /?
* fastcapture_streaming.exe Usage Info *

       fastcapture_streaming.exe
             [DirectoryPath_Options] [PcmShift_Options (default: 4)]
             [StreamingLength_Options (Limit the streaming to N seconds, default: 60)]
             [ExposureValue_Options (default: auto exposure)]

Examples:
       1 - fastcapture_streaming.exe -DirectoryPath C:\data\

       2 - fastcapture_streaming.exe -DirectoryPath C:\data\ -PcmShift 5 -StreamingLength 1000 -ExposureValue -3

       3 - fastcapture_streaming.exe -d C:\data\ -s 4 -l 60 -e -2
```