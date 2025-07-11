#!/usr/bin/env python3
# coding: utf-8
"""
Converts input image file to Stax/Flex/Apex image file
"""
# -----------------------------------------------------------------------------
import argparse
from PIL import Image, ImageOps
from icon2glyph import image_to_packed_buffer, gzlib_compress, convert_to_image_file, NbglFileCompression

def open_image(file_name: str, bpp: int) -> Image:
    if bpp == 1:
        img = Image.open(file_name).convert("1")  # 1-bit, black and white
        img = ImageOps.invert(img)
    else:
        img = Image.open(file_name).convert("L")  # 8-bit grayscale
    return img

# -----------------------------------------------------------------------------
# Program entry point:
# -----------------------------------------------------------------------------
if __name__ == "__main__":
    # -------------------------------------------------------------------------
    # Parse arguments:
    parser = argparse.ArgumentParser(
        description="Converts input image file to Stax/Flex/Apex image file")

    parser.add_argument(
        "-i", "--input",
        dest="input", type=str,
        required=True,
        help="bmp file")

    parser.add_argument(
        "-b", "--bpp",
        dest="bpp", type=int,
        required=True,
        help="number of bit per pixel (1, 2 or 4)")

    parser.add_argument(
        '--compress',
        action='store_true',
        default=False,
        help="compress data")

    parser.add_argument(
        '--file',
        action='store_true',
        default=False,
        help="store in Ledger image format")

    parser.add_argument(
      '--outfile',
      dest='outfile', type=str,
      required=False,
      help='Optional outfile name'
    )

    args = parser.parse_args()

    # Open image
    img = open_image(args.input, args.bpp)

    # Apply compression
    if args.compress:
      output_buffer = gzlib_compress(img, args.bpp, False)
    else:
      output_buffer = pixels_buffer

    # Apply file formatting
    if args.file:
        width, height = img.size
        result = convert_to_image_file(output_buffer, width, height, args.bpp, NbglFileCompression.Gzlib)
    else:
        result = output_buffer

    if args.outfile is None:
      # Print result
      print(result.hex())
    else:
      # Write to output file
      with open(args.outfile, 'wb') as out_file:
        print(f'Write {out_file.name}')
        out_file.write(bytes(result))
