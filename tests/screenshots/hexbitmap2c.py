#!/usr/bin/env python3

"""
Converts the given hexbitmap to a C array with the given name
"""

import argparse
import math
import os
import binascii
import sys
import traceback
import gzip

def main():
    parser = argparse.ArgumentParser(
        description='Converts the given hexbitmap to a C array with the given name and creates (or appends) in 2 files (C and H)')
    parser.add_argument('--hexbitmap', help="hexbitmap file", required = True)
    parser.add_argument('--src', help="output C file name")
    parser.add_argument('--inc', help="output H file name")
    parser.add_argument('--variable', help="name of C variable", required = True)
    args = parser.parse_args()

    # remove potential suffix
    mVar = args.variable.split('.')[0]
    # open hexbitmap file
    with open(args.hexbitmap, 'r') as hexbitmap_file:
        hexbitmap = hexbitmap_file.read()

        if args.src != None:
            # init string for .c file
            output = "const unsigned char %s[%d] = {"%(mVar, (len(hexbitmap)+1)/2)

            for i in range(0, len(hexbitmap)-1, 2):
                pair = hexbitmap[i:i+2]
                output += f"0x{pair}, "

            output = output[:-2]

            output += "};\n"

            # save .c file
            with open(args.src, 'a+') as output_file:
                output_file.write(output)

        if args.inc != None:
            # init string for .h file
            output = "extern const unsigned char %s[%d];\n"%(mVar, (len(hexbitmap)+1)/2)

            # add "#pragma once" if file is new
            if not os.path.exists(args.inc):
                with open(args.inc, 'w') as output_file:
                    output_file.write("#pragma once\n\n")
            # append in .h file
            with open(args.inc, 'a+') as output_file:
                output_file.write(output)

if __name__ == "__main__":
    main()
