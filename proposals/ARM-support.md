
# Support for ARM

* [x] Proposed //WES: I think we should add dates to these statuses when they get checked. Over time it will help to inform us how current / stale the doc is.
* [ ] Prototype: Not Started
* [ ] Implementation: Not Started
* [ ] Specification: Not Started

## Summary

Support the Sensor and Body Tracking SDKs on ARM based boards.

## Why we are doing this

Support for ARM is the most asked feature on customer voice and because you asked, we want to add it to our list of supported platforms!

## Supported Hardware

We are considering to use 2 kinds of Jetson boards for the ARM support:

- Jetson Nano
    - GPU: Custom 128 CUDA Core GPU
    - CPU: Quad-Core ARM Cortex-A57 @ 1.43 GHz
    - 4GB DDR4 RAM
- Jetson TX2 (to run Body Tracking SDK)
    - GPU: Custom 256 CUDA Core GPU (Pascal architecture)
    - CPU: Dual-Core NVIDIA Denver 2 ARMv8 64-bit processor + Quad-Core ARM Cortex-A57 @ 1.43 GHz
    - 4GB / 8GB DDR4 RAM

## Implementation

There are several large adjustments need to be implemented: //WES: there are 4 repo's that need to be modified, SDK, BT, Depth, and packaging.

- Changes to both Sensor SDK and Body Tracking SDK to make them run on ARM
- Changes to the build process for Sensor SDK, Depth Engine and Body Tracking SDK
- Changes to the test system for Sensor SDK, Depth Engine and Body Tracking SDK
- Changes to the release process for both Linux and Windows //WES: i think we only need to update the release process for Windows.

//WES: Questions - should we target release ARM binaries for Windows and Linux? or Just Linux? I am leaning toward just Ubuntu 18.04.
// WES: there is code in the depth engine that uses SSE instructions. This has been worked around with software equivalents. We should have an item to evaluate if we need to address it before release or not.


## Packaging

### Sensor SDK

- Microsoft installer (MSI)
- Nuget package
- Deb package

### Body Tracking SDK

- Microsoft installer (MSI)
- Nuget package
- Deb package
