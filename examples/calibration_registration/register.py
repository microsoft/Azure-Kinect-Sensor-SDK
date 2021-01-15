"""
An app script to run registration between two cameras from the command line.

Copyright (C) Microsoft Corporation. All rights reserved.
"""

# Standard Libraries.
import argparse

# Calibration tools.
from camera_tools import register

# ------------------------------------------------------------------------------
def parse_args():
  """
  Get arguments for running the registration.

  Returns:
    args -- Return a list of command line arguments for running registration.
  """

  parser = argparse.ArgumentParser(description="Get extrinsics for cameras.")
  parser.add_argument("-ia", "--img-a", required=True,
    help="Full path to image from camera A.")
  parser.add_argument("-ib", "--img-b", required=True,
    help="Full path to image from camera B.")
  parser.add_argument("-t", "--template", required=True,
    help="Full path to Charuco board template file.")
  parser.add_argument("-ca", "--calib-a", required=True,
    help="Full path to calibration file from camera A.")
  parser.add_argument("-cb", "--calib-b", required=True,
    help="Full path to calibration file from camera B.")
  parser.add_argument("-o", "--out-dir", required=True,
    help="Output directory for full calibration blob.")

  cmd_args = parser.parse_args()
  return cmd_args

if __name__ == "__main__":
  args = parse_args()

  rotation, translation, rms1_pixels, rms1_rad, rms2_pixels, rms2_rad = \
    register(args.img_a,
             args.img_b,
             args.template,
             args.calib_a,
             args.calib_b,
             args.out_dir)
