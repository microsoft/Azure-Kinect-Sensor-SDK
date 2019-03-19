The tool `calibration_info.exe` shows calibration information of all connected
devices. Using `calibration_info.exe` without any command line arguments will
display calibration info of all connected devices in stdout (the mode
`K4A_DEPTH_MODE_NFOV_UNBINNED` is used for depth camera calibration info). If
a device_id is given (0 for default device), the `calibration.json` file of
that device will be saved to the current directory. You can also specify the output file name with another optional parameter.

Examples:
- `calibration_info.exe` (prints out calibration info for all devices)
- `calibration_info.exe 1` (saves calibration blob for the second enumerated device to `calibration.json`)
- `calibration_info.exe 1 output.txt` (saves calibration blob for the second enumerated device to `output.txt`)

