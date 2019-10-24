
# Support for ARM

* [x] Proposed
* [ ] Prototype: Not Started
* [ ] Implementation: Not Started
* [ ] Specification: Not Started

## Summary

Support the Sensor and Body Tracking SDKs on ARM based boards.

## Why we are doing this

Support for ARM is the most asked feature on customer voice and because you asked, we want add it to our list of supported platforms!

## Supported Hardware

We are considrening 2 Jetson boards for the ARM support:

- Jetson Nano
    - GPU: Custom 128 CUDA Core GPU
    - CPU: Quad-Core ARM Cortex-A57 @ 1.43 GHz
    - 4GB DDR4 RAM
- Jetson TX2 (to run Body Tracking SDK)
    - GPU: Custom 256 CUDA Core GPU (Pascal architecture)
    - CPU: Dual-Core NVIDIA Denver 2 ARMv8 64-bit processor + Quad-Core ARM Cortex-A57 @ 1.43 GHz
    - 4GB / 8GB DDR4 RAM

## Packaging

### Sensor SDK

- Microsoft installer (MSI)
- Nuget package
- Deb package

### Body Tracking SDK

- Microsoft installer (MSI)
- Nuget package
- Deb package