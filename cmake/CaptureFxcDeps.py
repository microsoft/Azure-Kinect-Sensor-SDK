#!/usr/bin/env python3

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Calls the FXC compiler and generates a depfile from the compiler's standard output
# Lines in the standard output matching:
#   Resolved to [D:\foo\bar.h]
# Produce rules in the depfile


import re, argparse, sys, subprocess, os

parser=argparse.ArgumentParser(description='Caputure dependencies of the FXC compiler')
parser.add_argument('--outputs', metavar='OUTPUT', type=str, nargs='+', help='Outputs of the FXC compiler', required=True)
parser.add_argument('--depfile', type=str, help='Dep file path', required=True)
parser.add_argument('--fxc', type=str, help='path to FXC compiler', required=True)
parser.add_argument('--prefix', type=str, help='Root path of output files to strip', required=True)
parser.add_argument('fxcargs', metavar='arg', type=str, nargs='*', help='Arguments to the FXC compiler')

def main(argv):

    args = parser.parse_args()

    # Invoke the compiler

    proc = subprocess.Popen([args.fxc] + args.fxcargs, stdout=subprocess.PIPE, encoding="utf-8")

    proc.wait()


    stdout = proc.stdout.read()

    with open (args.depfile, 'w') as output:

        inputs = []
        for line in stdout.split(os.linesep):
            matchObj = re.match( r'Resolved to \[(.*)\]', line)
            if (matchObj):
                inputs.append(matchObj.group(1))

        for outfile in args.outputs:
            if (outfile.startswith(args.prefix)):
                outfile = outfile[len(args.prefix):]

            if (outfile.startswith('/')):
                outfile = outfile[1:]

            output.write("{} : {}\n".format( outfile, " ".join(inputs) ))

    # Print the output only on errors
    if (proc.returncode != 0):
        print([args.fxc] + args.fxcargs)
        print(stdout)
        print('Done ({})'.format(proc.returncode))

    sys.exit(proc.returncode)

if __name__ == "__main__":
    main(sys.argv[1:])
