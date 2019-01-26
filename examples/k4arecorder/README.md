# K4ARecorder

## Introduction

K4ARecorder is a command line utility for creating K4A device recordings. Recordings are saved in the Matroska (MKV) format,
using multiple tracks to store each sensor stream in a single file.

## Usage Info

```
k4arecorder [options] output.mkv

 Options:
  -h, --help           Prints this help
  --list               List the currently connected K4A devices
  --device             Specify the device index to use (default: 0)
  -l, --record-length  Limit the recording to N seconds (default: infinite)
  -c, --color-mode     Set the color sensor mode (default: 1080p), Available options:
                         3072p, 2160p, 1536p, 1440p, 1080p, 720p, 720p_NV12, 720p_YUY2, OFF
  -d, --depth-mode     Set the depth sensor mode (default: NFOV_UNBINNED), Available options:
                         NFOV_2X2BINNED, NFOV_UNBINNED, WFOV_2X2BINNED, WFOV_UNBINNED, PASSIVE_IR, OFF
  -r, --rate           Set the camera frame rate in Frames per Second
                         Default is the maximum rate supported by the camera modes.
                         Available options: 30, 15, 5
  --imu                Set the IMU recording mode (ON, OFF, default: ON)
```
