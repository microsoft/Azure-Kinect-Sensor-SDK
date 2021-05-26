"""
An app script for running calibration of a camera from the command line.

Copyright (C) Microsoft Corporation. All rights reserved.
"""

# Standard Libraries.
import argparse

# Calibration tools.
from camera_tools import calibrate_camera

# ------------------------------------------------------------------------------
def parse_args():
  """
  Get arguments for running the calibration.

  Returns:
    args -- Return a list of command line arguments for running calibration.
  """

  parser = argparse.ArgumentParser(description="Generate calibration.")
  parser.add_argument("-d", "--dataset-dir", required=True,
    help="Full path to all captures from camera.")
  parser.add_argument("-t", "--template", required=True,
    help="Full path to Charuco board template file.")
  parser.add_argument("-c", "--calib-file", default=None,
    help="Optional initial calibration file.")

  cmd_args = parser.parse_args()
  return cmd_args

if __name__ == "__main__":
  args = parse_args()

  rms, calib_matrix, dist_coeffs, img_size, rvecs, tvecs, num_good_images = \
    calibrate_camera(args.dataset_dir,
                     args.template,
                     init_calfile=args.calib_file)
  print(f"RMS: the overall RMS re-projection error in pixels: {rms}")
  print(f"\nopencv cal calibration matrix:\n{calib_matrix}")
  print(f"\nopencv cal distortion coeffs:\n{dist_coeffs}")
