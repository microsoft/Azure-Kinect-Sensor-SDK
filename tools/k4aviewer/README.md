# K4AViewer

## Introduction

K4AViewer is a graphical utility that allows you to start Azure Kinect devices in various modes and visualize the data from the sensors.
To use, select a device from the list, click "Open Device", choose the settings you want to start the camera with, then click "Start".

## Usage Info

k4aviewer will try to detect if you have a high-DPI system and scale automatically; however, if you want to force it into or out of
high-DPI mode, you can pass -HighDPI or -NormalDPI to override that behavior.

```
k4aviewer.exe [-HighDPI|-NormalDPI]
```