#!/usr/bin/env python3

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Used to run Google Tests generated during build. Takes in a text file
# containing a list of relative paths to text exectuables to run and path to
# binary output directory of build. The paths are relative to the binary
# directory.
#  ex: ./RunTestList.py --list bin/unit_test_list.txt --bin=bin/
#
# User may also pass extra args to gtest
import re, argparse, sys, subprocess, os

parser=argparse.ArgumentParser(description='Run test list of built k4a tests')
parser.add_argument('--list', type=str, help='Test list file path', required=True)
parser.add_argument('--bin', type=str, help='path to build binary directory', required=True)
parser.add_argument('--output', type=str, help='generate test result output', choices=['xml', 'json'])

def main(argv):

    [args, gtestargs] = parser.parse_known_args()
    gtestfullargs = gtestargs

    # Check binary directory
    if not os.path.isdir(args.bin):
        print(args.bin + " is not a directory")
        sys.exit(2)

    # Read test list
    if not os.path.isfile(args.list):
        print(args.list + " does not exist")
        sys.exit(1)
    with open(args.list) as f:
        test_list = f.read().splitlines()

    exitcode = 0
    for test in test_list:
        test = os.path.join(args.bin, test)
        if not os.path.isfile(test):
            print("Can not find test " + test)
            sys.exit(3)

        if args.output:
            output_arg = "--gtest_output={0}:TEST-{1}.{0}".format(args.output, os.path.splitext(os.path.basename(test))[0])
            gtestfullargs = [output_arg] + gtestargs

        print("Running test " + test)
        print(' '.join([test] + gtestfullargs))
        sys.stdout.flush()

        env = os.environ.copy()
        if "K4A_LOG_LEVEL" not in env.keys():
            env["K4A_LOG_LEVEL"] = "I"
        if "K4A_ENABLE_LOG_TO_STDOUT" not in env.keys():
            env["K4A_ENABLE_LOG_TO_STDOUT"] = "1"
        returncode = subprocess.call([os.path.abspath(test)] + gtestfullargs, stdout=sys.stdout, env=env, cwd=args.bin)
        # abspath is to avoid requiring './' on Linux
        if (returncode != 0):
            exitcode = returncode

    sys.exit(exitcode)

if __name__ == "__main__":
    main(sys.argv[1:])
