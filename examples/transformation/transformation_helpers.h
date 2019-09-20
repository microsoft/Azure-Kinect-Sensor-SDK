// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <k4a/k4a.h>

void tranformation_helpers_write_point_cloud(const k4a_image_t point_cloud_image,
                                             const k4a_image_t color_image,
                                             const char *file_name);

k4a_image_t downscale_image_2x2_binning(const k4a_image_t color_image);