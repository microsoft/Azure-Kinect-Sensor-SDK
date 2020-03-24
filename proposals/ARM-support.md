
# Support for ARM

## Proposal State

* [x] Proposed 10/31/2019
* [x] Prototype: Skipped
* [x] Implementation Started: 1/1/2020
* [x] Feature Complete: 3/23/2020

## Summary

Support the Sensor and Body Tracking SDKs on ARM based boards.

## Feature Scenario

Support for ARM enables our customers to build more stand along solutions utilizing mini- PC platforms. This unlocks more scenarios around robotics, manufacturing and healthcare where an Azure Kinect DK should be more mobile. The feature is also the most asked feature on customer voice.

## Supported Hardware

We are considering to use 2 kinds of Jetson boards for the ARM support since most of our customers prefer these boards:

- Jetson Nano
    - GPU: Custom 128 CUDA Core GPU
    - CPU: Quad-Core ARM Cortex-A57 @ 1.43 GHz
    - 4GB DDR4 RAM
- Jetson TX2 (to run Body Tracking SDK)
    - GPU: Custom 256 CUDA Core GPU (Pascal architecture)
    - CPU: Dual-Core NVIDIA Denver 2 ARMv8 64-bit processor + Quad-Core ARM Cortex-A57 @ 1.43 GHz
    - 4GB / 8GB DDR4 RAM

## Implementation

There are several large adjustments need to be implemented:

- Changes to Sensor SDK, Depth Engine, Body Tracking SDK and packaging repo's
- Changes to the build process for Sensor SDK, Depth Engine and Body Tracking SDK
- Changes to the test system for Sensor SDK, Depth Engine and Body Tracking SDK
- Changes to the release process for both Linux and Windows.
- Evaluate and make changes if needed to the depth engine code that uses SSE instructions.

We will target releasing ARM binaries for Windows and Linux.

## Packaging

### Sensor SDK

- Microsoft installer (MSI)
- Nuget package (for Windows)
- Deb package (For Ubuntu 18.04)

### Body Tracking SDK

- Microsoft installer (MSI)
- Nuget package (For Windows)
- Deb package (For Ubuntu 18.04)
