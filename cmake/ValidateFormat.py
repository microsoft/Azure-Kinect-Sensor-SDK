#!/usr/bin/env python3

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Invokes clang-format and reports formatting errors
# Optionally can cause clang-format to run the format in place with --reformat

import re, argparse, sys, subprocess, os.path, xml.dom.minidom

parser=argparse.ArgumentParser(description='Validate clang-fromat for files')
parser.add_argument('--file', metavar='INPUTFILE', type=str, nargs='+', help='Files to validate', required=True)
parser.add_argument('--output', metavar='OUTPUT', type=str, help='Output location', required=True)
parser.add_argument('--clangformat', type=str, help='path to the clang-format tool', required=True)
parser.add_argument('--reformat', action='store_true')
def main(argv):

    args = parser.parse_args()
    errorcount = 0
    for file in args.file:
        reformatcount = 0
        # Invoke clang-format
        if args.reformat:

            # Retry the reformat operation so long as the file continues to be modified
            # Sometimes clang-format will reformat a file, but running it again will apply additional changes
            filetime = os.path.getmtime(file)
            updatefile = True
            while (updatefile):
                proc = subprocess.Popen([args.clangformat] + ['-i', file], stdout=subprocess.PIPE, encoding="utf-8")
                stdout = proc.stdout.read()
                proc.wait()

                updatefile = False
                updatedtime = os.path.getmtime(file)
                
                if (updatedtime != filetime):
                    updatefile = True
                    print("clang-format updated {}".format(file))
                filetime = updatedtime
        else:
            proc = subprocess.Popen([args.clangformat] + ['-output-replacements-xml', file], stdout=subprocess.PIPE, encoding="utf-8")
            stdout = proc.stdout.read()
            proc.wait()

            # Parse the output
            dom = xml.dom.minidom.parseString(stdout)

            # Iterate through the file and count the number of replacements on each source line

            # Offset within the file
            offset = 0
            # Line number within the file
            lineno = 0

            # Current line with errors
            linewitherrors = 0
            replacementsonline = 0
            with open (file, 'r') as input:
                for replacement in dom.getElementsByTagName('replacement'):
                    reformatcount = reformatcount + 1
                    # replacement offset
                    roffset = int(replacement.getAttribute('offset'))

                    # advance the file one line at a time until the file offset
                    # is past the current replacement offset
                    while offset < roffset:
                        line = input.readline()
                        offset = input.tell()
                        lineno = lineno + 1

                    # If this replacement is on a different line from the last one,
                    # print the error information from the previous line
                    if linewitherrors != lineno and linewitherrors != 0:
                        err = "Error {} ({}): {} clang-format replacements on line".format(
                            file, 
                            linewitherrors,
                            replacementsonline
                            )

                        print(err)
                        replacementsonline = 0
                       
                    # Count the number of replacements on this line
                    linewitherrors = lineno
                    replacementsonline = replacementsonline + 1

            # Print the last line of replacment information if any
            if linewitherrors != 0:
                err = "Error {} ({}): {} clang-format replacements on line".format(
                    file, 
                    linewitherrors,
                    replacementsonline,
                    )
                print(err)
                print("     Run the clangformat target to auto-clean (e.g. \"ninja clangformat\")")

            errorcount = errorcount + reformatcount

    # Touch the output file if the operation was successful
    if errorcount == 0:
        with open (args.output, 'w') as output:
            output.write("")

    sys.exit(errorcount)

if __name__ == "__main__":
    main(sys.argv[1:])
