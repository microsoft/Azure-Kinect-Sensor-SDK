# K4A Fast Capture:streaming

## Introduction

K4A Fast Capture streaming process keeps streaming both the color and depth frames to the sensor and will store the most recent frames to the specified folder when notified by K4A Fast Capture trigger.

## Usage Info

       fastcapture_streaming.exe [DirectoryPath_Options] [CaptureNumber_Options] [ExposureValue_Options] [DepthShift_Options]
Examples:
       fastcapture_streaming.exe -DirectoryPath C:\data\ -CaptureNumber 10 -ExposureValue -3 -DepthShift 5
       fastcapture_streaming.exe -d C:\data\  -e -2 -s 4

