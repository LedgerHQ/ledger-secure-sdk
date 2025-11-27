#!/usr/bin/env python3
# coding: utf-8
"""
@BANNER@
"""
# -----------------------------------------------------------------------------
import argparse
import sys
import os
import json
import base64
import struct
import configparser
from fontTools.ttLib import TTFont
from PIL import Image, ImageFont, ImageDraw
from rle_custom import RLECustom

# -----------------------------------------------------------------------------
class TTF2INC:
    """
    This class will contain methods to handle TTF fonts & generate .inc files
    """
    # Default values:
    FONT_NAME = "open_sans_regular.ttf"
    FONT_SIZE = 11
    LINE_SIZE = 16
    FIRST_CHARACTER = 0x20
    LAST_CHARACTER = 0x7E
    # Maximum possible bytes to skip when crop is False
    # (x_min_offset & y_min_offset contain the nb of skipped bytes on 6 bits)
    MAX_SKIPPED_BYTES = 63
    # Default string containing all supported unicode characters
    # Mandatory character
    STRING = "�"
    # Add French specific characters
    STRING += "àâäæçèéêëîïôœùûüÀÂÄÆÇÈÉÊËÎÏÔŒÙÛÜ"
    # Add Spanish specific characters (not already included)
    STRING += "¡¿ÁÍÑÓÚáíñóú"
    # Add German specific characters (not already included)
    STRING += "Ößö"
    # Add Portuguese specific characters (not already included)
    STRING += "ÃÕãõ"
    # Add Turkish specific characters (not already included)
    STRING += "İıĞğŞş"
    # Add Russian specific characters (not already included)
    STRING += "АаБбВвГгДдЕеЁёЖжЗзИиЙйКкЛлМмНнОоПпРрСсТтУуФфХхЦцЧчШшЩщЪъЫыЬьЭэЮюЯя"
    # Add Thai specific characters (not already included)
    STRING += "กขฃคฅฆงจฉชซฌญฎฏฐฑฒณดตถทธนบปผฝพฟภมยรฤลฦวศษสหฬอฮฯะัาำิีึืฺุู฿เแโใไๅๆ" \
        "็่้๊๋์ํ๎๏๐๑๒๓๔๕๖๗๘๙๚๛"

    # -------------------------------------------------------------------------
    def __init__(self, args):
        super().__init__()
        # Default values:
        self.nbgl = False
        self.rle = False
        self.crop = True
        self.apex = False
        self.nanos = False
        # This flag is very important, as it allows us to determine whether we:
        # - False: generate only ASCII bitmaps (0x20 to 0x7E), using only main font data
        # - True : generate unicode bitmaps, meaning we may use any font data
        # => it is updated in 'find_string_dimension' method, when parsing all characters
        self.unicode_needed = False
        self.unicode_index = 0 # It is the index of the font section to use!
        self.font = None
        self.ttfont = None
        self.name = None
        self.font_id_name = None
        self.basename = None
        self.directory = None
        self.left = None
        self.right = None
        self.top = None
        self.bottom = None
        self.font_prefix = None
        self.font_height = 0
        self.real_height = 0
        self.font_size = 0
        self.line_size = 0
        self.kerning = 0
        self.char_leftmost_x = 0
        self.max_y_min_offset = 0
        self.max_x_min_offset = 0
        self.max_x_max_offset = 0
        self.char_topmost_y = 0
        self.bitmap_len = 0
        self.loaded_baseline = 0
        self.baseline_offset = 0
        self.bpp = args.bpp
        self.font_index = -1
        self.char_info = {}
        self.ttf_info_dictionary = {}
        # Be sure there are at least some mandatory characters
        self.unicode_chars = "�"
        self.ids = []
        self.font_name = []
        self.font_sizes = []
        self.font_heights = []
        self.line_sizes = []
        self.rles = []
        self.crops = []
        # Store parameters:
        self.args = args
        # Initialise config with provided values or default ones
        self.init_config()
        # Get font infos
        self.fonts = []
        self.ttfonts = []
        self.baselines = []

        if self.args.verbose:
            sys.stderr.write(f"self.font_name={self.font_name}\n")
        for index, font_name in enumerate(self.font_name):
            if not os.path.exists(font_name):
                sys.stderr.write(f"Can't open file {font_name}\n")
                sys.exit(-2)
            try:
                self.font = ImageFont.truetype(font_name, self.font_sizes[index], layout_engine=0)
                self.fonts.append(self.font)
                ascent, descent = self.font.getmetrics()
                self.baselines.append(ascent)
                self.font_heights.append(self.font.font.height)

                if self.args.verbose:
                    sys.stderr.write(f"{font_name} metrics: ascent={ascent}, "
                                     f"descent={descent}, height={self.font.font.height}\n")
                ttfont = TTFont(font_name)
                self.ttfonts.append(ttfont)
            except (BaseException, Exception) as error:
                sys.stderr.write(f"Error with font {font_name}: {error}\n")
                sys.exit(-1)

        # Select font 0 by default and update all global info
        self.update_font_info(0)
        self.char_leftmost_x = self.font_width
        self.char_topmost_y = self.font_height

    # -------------------------------------------------------------------------
    def __enter__(self):
        """
        Return an instance to this object.
        """
        return self

    # -------------------------------------------------------------------------
    def __exit__(self, exec_type, exec_value, traceback):
        """
        Do all the necessary cleanup.
        """

    # -------------------------------------------------------------------------
    def update_font_info(self, index):
        """
        Update the font related variables according to index value.
        WARNING: some values depends on which kind of data we will generate (ASCII/UNICODE)
        """
        self.font_index = index
        self.font = self.fonts[index]
        self.ttfont = self.ttfonts[index]
        self.name = self.font_name[index]
        self.font_size = self.font_sizes[index]

        # Get font metrics:
        # - ascent: distance from the baseline to the highest outline point.
        # - descent: distance from the baseline to the lowest outline point.
        # (descent is a negative value)
        self.ascent, self.descent = self.font.getmetrics()
        self.font_width = self.font_size   # This is just an approximation
        self.font_height = self.font_heights[index]
        self.baseline = self.baselines[index]

        # if self.unicode_needed is not False, then we want to generate UNICODE data
        if index == 0 and self.unicode_needed and len(self.rles) > 1:
            index = 1   # Use rle & crop values from first section after main

        self.line_size = self.line_sizes[index]

        self.rle = self.rles[index]
        self.crop = self.crops[index]

    # -------------------------------------------------------------------------
    def get_font_id_name(self):
        """
        Return the name of the font id.
        """
        # If we provided one, just return it
        if self.args.font_id_name:
            return self.args.font_id_name

        # Those ids are defined in bagl.h file, in bagl_font_id_e enums.
        font_id_name = "BAGL_FONT"
        font_name = self.font_name[0].lower()
        if "open" in font_name and "sans" in font_name:
            font_id_name += "_OPEN_SANS"
            if "regular" in font_name:
                font_id_name += "_REGULAR"
            elif "extrabold" in font_name:
                font_id_name += "_EXTRABOLD"
            elif "semibold" in font_name:
                font_id_name += "_SEMIBOLD"
            elif "light" in font_name:
                font_id_name += "_LIGHT"
        else:
            font_id_name += "_UNKNOWN"

        font_id_name += f"_{self.font_size}px"

        return font_id_name

    # -------------------------------------------------------------------------
    def init_config(self):
        """
        Initialise all important values using:
        - .ini file if we provided one
        - command line parameters
        - default values otherwise
        """
        # Read config from the .ini file if we provided one
        if self.args.init_file:
            config_parser = configparser.ConfigParser()
            config_parser.read(self.args.init_file)
            print(self.args.init_file)
            # Get main settings
            config = config_parser['main']
            self.font_name = config.get('font').split()
            self.font_id_name = config.get('font_id_name')
            self.loaded_baseline = int(config.get('loaded_baseline', 0))
            self.baseline_offset = int(config.get('baseline_offset', 0))
            self.first_character = int(config.get('firstChar', '0x20'), 16)
            self.last_character = int(config.get('lastChar', '0x7E'), 16)
            self.font_sizes.append(config.getint('fontSize'))
            self.line_sizes.append(config.getint('lineSize'))
            self.crops.append(config.getboolean('crop', False))
            self.kerning = config.getint('kerning', 0)
            self.rles.append(config.getboolean('rle', True))
            self.bpp = config.getint('bpp', 1)
            self.nbgl = config.getboolean('nbgl', False)
            # Get additional settings, if any
            for section in config_parser.sections():
                if section == 'main':
                    continue    # already done
                if self.args.verbose:
                    sys.stderr.write(f"Reading section [{section}]\n")
                config = config_parser[section]
                # Get mandatory values
                self.font_name.append(config.get('font'))
                # Get other values, if present, or use [main] section ones
                self.font_sizes.append(config.getint('fontSize', self.font_sizes[0]))
                self.line_sizes.append(config.getint('lineSize', self.line_sizes[0]))
                self.rles.append(config.getboolean('rle', self.rles[0]))
                self.crops.append(config.getboolean('crop', self.crops[0]))

        # Otherwise, use command line parameters (or default values)
        else:
            self.font_name = self.args.font_name.split()
            self.font_id_name = self.get_font_id_name()
            self.first_character = self.args.first_character
            self.last_character = self.args.last_character
            self.font_sizes.append(self.args.font_size)
            self.line_sizes.append(self.args.line_size)

        if self.nbgl:
            self.font_prefix = "nbgl_font_"
        else:
            self.font_prefix = "bagl_font_"

        # Be sure filenames for fonts are correct, and update them if necessary
        self.find_fonts()

        # Update Apex flag
        self.get_font_id()

    # -------------------------------------------------------------------------
    def find_fonts(self):
        """
        Try to find where are located the fonts specified in self.font_name
        (in current directory, in the .ini directory or in the .py directory)
        """
        # Make self.font_name a list with all font names
        if not isinstance(self.font_name, list):
            self.font_name = self.font_name.split()

        for index, font_name in enumerate(self.font_name):

            # Check first in current directory
            if os.path.exists(font_name):
                # We found it!!
                continue

            # Check at .ini location
            if self.args.init_file:
                filename = os.path.dirname(self.args.init_file)
                filename = os.path.join(filename, font_name)
                if os.path.exists(filename):
                    # We found it!!
                    self.font_name[index] = filename
                    continue

            # Check at the location of the script
            filename = os.path.dirname(__file__)
            filename = os.path.join(filename, font_name)
            if os.path.exists(filename):
                # We found it!!
                self.font_name[index] = filename
                continue

        # If we didn't found the specified font, let pillow search by itself
        # (it looks in system directories too)

    # -------------------------------------------------------------------------
    def build_names(self):
        """
        Build filenames depending on font characteristics.
        """
        # Get the font basename, without extension
        self.basename = os.path.splitext(
            os.path.basename(self.name))[0]
        self.basename = self.basename.replace("-", "_")
        self.basename += f"_{self.font_size}px"
        if self.nbgl and self.bpp != 4:
            self.basename += f"_{self.bpp}bpp"
        if self.unicode_needed:
            self.basename += "_unicode"
        # Build destination directory name, based on font+size[+unicode]+bpp
        self.directory = os.path.dirname(self.name)
        name = self.font_prefix + self.basename
        self.directory = os.path.join(self.directory, name)
        # Create that directory if it doesn't exist
        if not os.path.exists(self.directory) and self.args.save:
            if self.args.verbose:
                sys.stdout.write(f"Creating directory {self.directory}\n")
            os.mkdir(self.directory)

    # -------------------------------------------------------------------------
    def char_is_in_font(self, char, ttfont):
        """
        Find if a char is the current font
        """
        for cmap in ttfont['cmap'].tables:
            if cmap.isUnicode():
                if ord(char) in cmap.cmap:
                    return True
        return False

    # -------------------------------------------------------------------------
    def find_char(self, char):
        """
        Update the index of font containing this character, searching first in font #0
        """
        index = 0
        # If it is an ASCII character, then it is present in font #0
        if ord(char) > 0x7F:

            # If the character is present in font #0, then we can use it
            while index < len(self.ttfonts):
                # Find the font having that character
                if self.char_is_in_font(char, self.ttfonts[index]):
                    if index > self.unicode_index:
                        self.unicode_index = index # Keep track of highest section # used
                    break

                index += 1

            if index >= len(self.ttfonts):
                # We didn't found that character => use font 0 by default
                if ord(char) != 0xFFFD:
                    sys.stderr.write(f"WARNING: The character '{char}' (0x{ord(char):X})"
                                     " was not found in any font!\n")
                index = 0

        # Update font information with index of the font to use
        self.update_font_info(index)

    # -------------------------------------------------------------------------
    def compute_check_bits(self):
        """
        Most offsets are stored on bitfields => compute values used to check they are big enough
        """
        # Compute value of max_x_min_offset, depending of allowed bits
        max_bits = 4
        self.max_x_min_offset = pow(2, max_bits) - 1

        # Compute value of max_x_max_offset, depending of allowed bits
        if self.nbgl:
            max_bits = 4
        else:
            if self.unicode_needed:
                max_bits = 5
            else:
                max_bits = 4
        self.max_x_max_offset = pow(2, max_bits) - 1

        # Compute value of max_y_min_offset, depending of allowed bits
        if self.nbgl:
            max_bits = 6
            self.max_y_min_offset = pow(2, max_bits) - 1
        else:
            if self.unicode_needed:
                max_bits = 6
            else:
                max_bits = 5
            self.max_y_min_offset = 4 * pow(2, max_bits) - 1

    # -------------------------------------------------------------------------
    def find_string_dimension(self, string):
        """
        Find the real dimension needed by all the characters in string.
        Update the unicode_needed flag and build filenames.
        """
        for char in string:

            # Update font info
            self.find_char(char)

            # Get dimension using font.getbbox
            left, top, right, bottom = self.font.getbbox(char)
            # Here, bottom is just ymax => make it be equal to ymax + 1
            bottom += 1

            # Some characters (ie 'j' & '_') may need to be displayed at x < 0
            # For Thai, Korean or Arabic, some chars may be displayed over previous one.
            # Thai characters that are displayed completely over previous one:
            # (left is << 0 and right is ~0)
            # 000E31, 000E34, 000E35, 000E36, 000E37, 000E38, 000E39
            # 000E3A, 000E47, 000E48, 000E49, 000E4A, 000E4C, 000E4D, 000E4E
            # Thai characters that are displayed left, overlapping a bit:
            # (left is < 0 and right is >> 0)
            # 000E07(-1,19), 000E42(-1, 12), 000E43(-1, 11), 000E44(-1, 11)
            # Between the two: 000E33 (-9, 16)
            over_previous = 0
            if left < 0:
                left_offset = left
                left = 0
                if left_offset < -2:
                    over_previous = 1
            else:
                left_offset = 0

            # Keep track of the top most Y for that font
            if top < self.char_topmost_y:
                self.char_topmost_y = top

            if top < 0 and self.baseline_offset < (-self.char_topmost_y):
                sys.stderr.write(f"WARNING: Font {self.font_id_name} "
                                 f"display character '{char}' ({ord(char):06X}) at y={top} "
                                 f"=> 'baseline_offset' value should be set "
                                 f"at least to {-(top)} !\n")

            # Some characters may be displayed over self.font_height
            if bottom > self.font_height:
                self.font_height = bottom
                self.font_heights[self.font_index] = self.font_height

            # Store relevant information
            self.char_info[char] = {
                "index": self.font_index,
                "over_previous": over_previous,
                "left_offset": left_offset,
                "left": left,
                "top": top,
                "right": right,
                "bottom": bottom
            }
            if self.args.verbose:
                sys.stderr.write(f"char (font): {char} ({ord(char):06X}) "
                                 f"left_offset={left_offset:3} (OP={over_previous}), left={left:2}"
                                 f",right={right:2} "
                                 f"top= {top:2}, bottom={bottom}, font_height={self.font_height}\n")

        # Make font_height be modulo 4, when crop is False
        if not self.crop:
            for index, font_height in enumerate(self.font_heights):
                font_height = (font_height + 3) & 0xFC
                self.font_heights[index] = font_height

        self.compute_check_bits()

        # char_topmost_y will be updated with the real information (from bitmap data)
        self.char_topmost_y = self.font_height

    # -------------------------------------------------------------------------
    def create_empty_image(self, width, height):
        """
        Create an image using font characteristics and provided dimensions.
        """
        if self.bpp == 1:
            background_color = 'black'
            text_color = 'white'
            # Use 'L' mode, because the conversion to 1 bpp is terrific!
            mode = "L"
        else:
            background_color = 'white'
            text_color = 'black'
            mode = "L"

        img = Image.new(mode, (width, height), color=background_color)

        return img, text_color

    # -------------------------------------------------------------------------
    def get_char_image(self, char):
        """
        Load or generate the image corresponding to the provided character(s).
        """
        # Get the unicode value of that char:
        # (in fact there is a plane value from 0 to 16 then a 16-bit code)
        unicode_value = f"0x{ord(char):06X}"

        # Build full path of the destination file:
        filename = f"{self.font_prefix}{self.basename}_{unicode_value}"
        filename_without_unicode = filename.replace('_unicode', '')
        fullpath = os.path.join(self.directory, filename)
        fullpath_without_unicode = os.path.join(self.directory, filename_without_unicode)

        # We'll look for .bmp, .png and .gif extensions to check if file exists
        for ext in [".bmp", ".png", ".gif"]:
            image_name = fullpath + ext
            if os.path.exists(image_name):
                break
            # try without 'unicode' in filename
            image_name = fullpath_without_unicode + ext
            if os.path.exists(image_name):
                break

        info = self.char_info[char]
        # STEP 1: Generate the image if it doesn't exist
        if not os.path.exists(image_name) or self.args.overwrite:
            if self.args.verbose:
                sys.stdout.write(f"Generating {image_name}\n")

            # To keep compatibility with 'genuine' generated images, picture
            # will have the height of the font and use font.getbbox sizes
            # Build an image with font height & taking in account left_offset
            width = info['right']
            if info['left_offset'] != 0:
                # Increase width by the number of negative pixels
                width -= info['left_offset']
                # Update right coordinate
                # info['right'] = width
                # No need to update 'right', as we can now display at x < 0

            height = self.font_height

            # Generate and save the picture
            # Create a B&W (or grey levels) image with that size
            img, text_color = self.create_empty_image(width, height)
            draw = ImageDraw.Draw(img)
            # Compute display coordinates, using real size of the character
            # (no pixel should be displayed outside of [width, height])
            offset_x = 0 - info['left_offset']
            offset_y = self.baseline_offset
            offset_y += self.baselines[0] - self.baseline

            # Draw the character
            draw.text((offset_x, offset_y), char, font=self.font,
                      fill=text_color)

            # We used 'L' mode for a better quality => convert image to 1 bpp
            if self.bpp == 1:
                # Convert the picture (ImageMagick may obtain a better quality)
                img = img.convert('1', dither=Image.Dither.NONE)

            # Save the picture if we asked to
            if self.args.save:
                image_name = change_ext(image_name, ".png")
                if self.args.verbose:
                    sys.stdout.write(f"Saving {image_name} ({width}x{height})"
                                     " (offset_x={offset_x}, offset_y={offset_y})\n")
                img.save(image_name)
        else:
            # We will load the existing picture
            if self.args.verbose:
                sys.stdout.write(f"Loading {image_name}\n")
            img = Image.open(image_name)
            # Be sure it is a B&W or grey levels picture:
            if self.bpp == 1:
                mode = "1"
            else:
                mode = "L"
            if img.mode != mode:
                # Convert the picture (ImageMagick may obtain a better quality)
                img = img.convert(mode, dither=Image.Dither.NONE)

            # Make sure the loaded picture have same characteristics than
            # the ones already used for the font (height, baseline etc)
            # => char bitmap should be displayed at the correct coordinates.
            width, height = img.size
            if height < self.font_height or \
               self.loaded_baseline != 0 or self.baseline_offset != 0:
                height = self.font_height
                # Create a new picture with expanded height
                new_img, _ = self.create_empty_image(width, height)
                # Integrate baseline (take in account baseline differences)
                if self.loaded_baseline != 0:
                    offset_y = self.baseline - self.loaded_baseline
                else:
                    offset_y = 0
                offset_y += self.baseline_offset
                offset_y += self.baselines[0] - self.baseline
                new_img.paste(img, (0, offset_y))
                img = new_img

            # Save the picture if we asked to
            if self.args.save:
                image_name = change_ext(image_name, ".png")
                if self.args.verbose:
                    sys.stdout.write(f"Saving {image_name} ({width}x{height})\n")
                img.save(image_name)

            # Let suppose picture correctly takes in account left_offset
            # (if it was generated using checkpics.py script, it is the case)
            # Keep width but check other real coordinates from the bounding box
            box = self.getbbox(img)
            if box is None:
                left = 0
                top = 0
                bottom = height
            else:
                left, top, _, bottom = box

            info['left_offset'] = 0
            info['left'] = left
            info['top'] = top
            info['right'] = width       # right is equal to width for the moment
            info['bottom'] = bottom

        # Update the dictionary containing character information
        info['img'] = img
        info['width'] = width
        info['height'] = height
        self.char_info[char] = info

    # -------------------------------------------------------------------------
    def box_to_packed_bitmap(self, img, left, top, right, bottom):
        """
        Store the pixel values of the specified box, in little endian encoding.
        (left pixels are stored starting at bit 0, grouped by bytes)
        """
        current_byte = 0
        current_bit = 0
        image_data = []

        if self.nbgl:
            nb_colors = pow(2, self.bpp)
            base_threshold = int(256 / nb_colors)
            half_threshold = int(base_threshold / 2)
            # Rotate & revert X axes on Stax
            for coord_x in reversed(range(left, right)):
                for coord_y in range(top, bottom):
                    color_index = img.getpixel((coord_x, coord_y))
                    color_index = \
                        int((color_index + half_threshold) / base_threshold)

                    if color_index >= nb_colors:
                        color_index = nb_colors - 1

                    # le encoded
                    current_byte += color_index << ((8-self.bpp)-current_bit)
                    current_bit += self.bpp

                    if current_bit >= 8:
                        image_data.append(current_byte & 0xFF)
                        current_bit = 0
                        current_byte = 0
        else:
            # bottom is actually Y2+1 => perfect for range scan
            for coord_y in range(top, bottom):
                # right is actually X2+1 => perfect for range scan
                for coord_x in range(left, right):
                    color_index = img.getpixel((coord_x, coord_y))
                    if color_index >= 128:
                        color_index = 1
                    else:
                        color_index = 0

                    # le encoded
                    current_byte += color_index << current_bit
                    current_bit += 1

                    if current_bit >= 8:
                        image_data.append(current_byte & 0xFF)
                        current_bit = 0
                        current_byte = 0

        # Handle last byte if any
        if current_bit > 0:
            image_data.append(current_byte & 0xFF)

        # Remove final transparent pixels, if any
        background_color = 0
        if self.nbgl and self.bpp != 1:
            background_color = 0xFF

        while self.rle and len(image_data) != 0 and image_data[-1] == background_color:
            image_data = image_data[:-1]

        # When crop is False, we can crop at bytes level
        skipped_bytes = 0
        if not self.crop:
            while len(image_data) != 0 and \
                  image_data[0] == background_color and \
                  skipped_bytes < self.MAX_SKIPPED_BYTES:
                image_data = image_data[1:]
                skipped_bytes += 1

        return skipped_bytes, bytes(image_data)

    # -------------------------------------------------------------------------
    def getbbox(self, img):
        """
        Return a 4-tuple defining the left, upper, right, and lower pixel
        coordinate, by taking in account the transparent color.
        (unlike what does Image.getbbox() method from Pillow)
        """
        width, height = img.size
        left = width
        top = height
        right = 0
        bottom = 0
        # Find transparent color (not 0 on Stax with 4BPP)
        if self.bpp == 1:
            background_color = 0
        else:
            background_color = 0xFF

        # Find left, top, right and bottom values for existing pixels
        for y_coord in range(height):
            for x_coord in range(width):
                if img.getpixel((x_coord, y_coord)) == background_color:
                    # This pixel is transparent => ignore it
                    continue

                if x_coord < left:
                    left = x_coord
                if x_coord >= right:
                    right = x_coord+1   # Right is X2+1 (right - left = width)

                if y_coord < top:
                    top = y_coord
                if y_coord >= bottom:
                    bottom = y_coord+1  # Bottom is Y2+1 (bottom - top = height)

        # Is it an empty box?
        if right <= left or bottom <= top:
            return None

        # Don't make bottom be modulo 4 on LNX, LNS+ & Apex
        if not self.apex and not self.nanos:
            bottom = (bottom+3) & 0xFC
            if bottom > height:
                bottom = height

        # Return the box coordinates in a 4-tuple
        return (left, top, right, bottom)

    # -------------------------------------------------------------------------
    def get_char_bitmap(self, char):
        """
        Return the packed bitmap corresponding to the provided character(s).
        """
        # Get the image that was generated or loaded from an existing picture
        info = self.char_info[char]
        img = info['img']

        # Get the bounding box (non-transparent region) of img
        box = self.getbbox(img)

        # Is it an empty box?
        if box is None:
            right = 0
            left = 0
            top = 0
            bottom = 0
            height = 0
        else:
            if self.crop:
                left, top, right, bottom = box
                if False and not self.nbgl:
                    # Don't modify right on BAGL (char spacing)
                    right = info['right']
            else:
                left = 0
                top = 0
                right = info['right']
                bottom = self.font_height

            # bottom is actually ymax + 1
            height = bottom - top

            # Are some values too big to fit in a small bitfield?
            # If yes, add transparent pixels to make the value fit
            if self.crop:
                # Be sure y_min_offset value is not too big
                if top > self.max_y_min_offset:
                    if self.args.verbose:
                        sys.stderr.write(f"Adjusting top from {top} to "
                                         f"{self.max_y_min_offset}"
                                         f" for char {char} ({ord(char):06X})\n")
                    # Compression will compensate those added transparent lines
                    top = self.max_y_min_offset

                # Be sure x_min_offset value is not too big
                if left > self.max_x_min_offset:
                    if self.args.verbose:
                        sys.stderr.write(f"Adjusting left from {left} to "
                                         f"{self.max_x_min_offset}"
                                         f" for char {char} ({ord(char):06X})\n")
                    # Compression will compensate those added transparent pixels
                    left = self.max_x_min_offset

                # Be sure difference between right & width is not too big
                if (info['width'] - right) > self.max_x_max_offset:
                    if self.args.verbose:
                        sys.stderr.write(f"Adjusting right from {right} to "
                                         f"{info['width'] - self.max_x_max_offset}"
                                         f" for char {char} ({ord(char):06X})\n")
                    # Compression will compensate those added 'empty' pixels
                    right = info['width'] - self.max_x_max_offset

            # Update offsets used by all chars in this font
            if left < self.char_leftmost_x:
                self.char_leftmost_x = left

            # bottom is actually ymax + 1
            if bottom > self.real_height:
                self.real_height = bottom

            if bottom > self.line_size:
                sys.stderr.write(f"WARNING: char '{char}' ({ord(char):06X}) is bigger "
                                 f"({bottom}) than line_size ({self.line_size})!\n")

            if top < self.char_topmost_y:
                self.char_topmost_y = top

        # Store the information to process it later
        info['left'] = left
        info['top'] = top
        info['right'] = right
        info['bottom'] = bottom # bottom is actually ymax + 1
        info['height'] = height

        # Get the packed bitmap corresponding to this box
        if height:
            skipped_bytes, img_data = self.box_to_packed_bitmap(
                img, left, top, right, bottom)
            size = len(img_data)
        else:
            img_data = None
            skipped_bytes = 0
            size = 0

        # Number of bytes that was skipped at the beginning of data
        info['skipped'] = skipped_bytes

        encoding = 0
        # Do we want to compress this font?
        if self.rle and img_data is not None:
            method, compressed_data = RLECustom.encode(img_data, self.bpp)
            # Is compressed size really better?
            # (for the moment, we enforce RLE even if it is a little bigger)
            if len(compressed_data) < len(img_data) or True:
                img_data = compressed_data
                size = len(img_data)
                encoding = method

        info['encoding'] = encoding
        info['bitmap'] = img_data
        info['size'] = size
        info['offset'] = 0

        self.char_info[char] = info

        if self.args.verbose:
            sys.stderr.write(f"char (img): {char} ({ord(char):06X}) "
                             f"width:{info['width']:2}, height:{height:2} "
                             f"left_offset={info['left_offset']:3}, left={left:2}, right={right:2} "
                             f"top={top:2}, bottom={bottom:2} "
                             f"datasize={size}, encoding={encoding}\n")

    # -------------------------------------------------------------------------
    def check_max_bits(self, value, max_bits, char, name):
        """
        Check if a provided value exceeds the number of bits allowed.
        """
        if value < 0 or value >= pow(2, max_bits):
            sys.stderr.write(f"Field '{name}' for char 0x{ord(char):X}({char}) in {self.basename}"
                             f" needs too much bits: value is {value} and"
                             f" {max_bits} bits are allowed!\n")
            info = self.char_info[char]
            sys.stderr.write(f"self.char_topmost_y={self.char_topmost_y}, info={info}\n")
            sys.stderr.write(f"Font ID: {self.font_id_name} ({self.font_name[info['index']]})")
            sys.exit(3)

    # -------------------------------------------------------------------------
    def save_nbgl_font(self, inc, crop, suffix, first_char, last_char):
        """
        Save the nbgl_font_t info into the .inc file.
        Structure defined in public_sdk/lib_nbgl/include/nbgl_fonts.h
        typedef struct {
          uint32_t bitmap_len;      ///< Size in bytes of the associated bitmap
          uint8_t font_id;          ///< ID of the font, from @ref nbgl_font_id_e
          uint8_t bpp;              ///< number of bits per pixels
          uint8_t height;           ///< height of all characters in pixels
          uint8_t line_height;      ///< height of a line for all characters in pixels
          uint8_t char_kerning;     ///< kerning for the font
          uint8_t crop;             ///< If false, x_min_offset+y_min_offset=bytes to skip
          uint8_t y_min;            ///< Most top Y coordinate of any char in the font
          uint8_t first_char;       ///< ASCII code of the first character
          uint8_t last_char;        ///< ASCII code of the last character
          const nbgl_font_character_t *const characters; ///< array with definitions of all chars
          uint8_t const *bitmap;    ///< array containing bitmaps of all characters
        } nbgl_font_t;
        """
        inc.write(
            "\n__attribute__ ((section(\"._nbgl_fonts_\"))) const "
            f"nbgl_font_t font{self.basename.upper()}{suffix}"
            f"= {{\n"
            f"  {self.bitmap_len}, // bitmap len\n"
            f"  {self.font_id_name}, // font id\n"
            f"  (uint8_t) NBGL_BPP_{self.bpp}, // bpp\n"
            f"  {self.real_height}, // height of all characters in pixels\n"
            f"  {self.line_sizes[self.unicode_index]}, // line height in pixels\n"
            f"  {self.kerning}, // kerning\n"
            f"  {crop}, // crop enabled (1) or not (0)\n"
            f"  {self.char_topmost_y}, // Most top Y coordinate of any char\n"
            f"  0x{first_char:X}, // first character\n"
            f"  0x{last_char:X}, // last character\n"
            f"  characters{self.basename.upper()},\n"
            f"  bitmap{self.basename.upper()}\n")

    # -------------------------------------------------------------------------
    def save_nbgl_font_character(self, inc, char, info):
        """
        Save the nbgl_font_character_t info into the .inc file, but first
        check that the values do not exceed the boundaries of each fieds.
        Structure defined in public_sdk/lib_nbgl/include/nbgl_fonts.h
        typedef struct {
          uint32_t bitmap_offset;  ///< offset of this character in chars buffer
          uint32_t encoding : 1;        ///< method used to encode bitmap data
          uint32_t width : 6;           ///< width of character in pixels
          uint32_t x_min_offset : 4;    ///< x_min = x_min_offset
          uint32_t y_min_offset : 6;    ///< y_min = (y_min + y_min_offset)
          uint32_t x_max_offset : 4;    ///< x_max = width - x_max_offset
          uint32_t y_max_offset : 6;    ///< y_max = (height - y_max_offset)
        } nbgl_font_character_t;
        """
        encoding = info["encoding"]
        bitmap_offset = info["offset"]
        bitmap_byte_count = info["size"]
        width = info["width"]

        # If it's an empty box, just put everything at 0
        if info["bitmap"] is None or len(info["bitmap"]) == 0:
            x_min_offset = 0
            y_min_offset = 0

            x_max_offset = 0
            y_max_offset = 0
        else:
            x_min_offset = info["left"]
            y_min_offset = info["top"] - self.char_topmost_y

            x_max_offset = width - info["right"]
            # bottom is actually ymax + 1
            y_max_offset = self.real_height - info["bottom"]

        # When crop is False, we may have some bytes to skip at beginning
        if not self.crop:
            skipped_bytes = info["skipped"]
            self.check_max_bits(skipped_bytes, 6, char, "skipped bytes")
            # We'll store 6 bits of skipped bytes in x_min_offset & y_min_offset
            x_min_offset = (skipped_bytes >> 3) & 7
            y_min_offset = skipped_bytes & 7

        # Check values does not exceed bitfield capabilities
        self.check_max_bits(encoding, 1, char, "encoding")
        self.check_max_bits(bitmap_offset, 15, char, "bitmap_offset")
        self.check_max_bits(width, 6, char, "width")
        self.check_max_bits(x_min_offset, 4, char, "x_min_offset")
        self.check_max_bits(y_min_offset, 6, char, "y_min_offset")
        # Next one should never occur, thanks to max_x_max_offset check
        self.check_max_bits(x_max_offset, 4, char, "x_max_offset")
        self.check_max_bits(y_max_offset, 6, char, "y_max_offset")

        inc.write(f"  {{ {bitmap_offset:5}, {encoding:1}, {width:2},"
                  f"{x_min_offset}, {y_min_offset}, "
                  f"{x_max_offset}, {y_max_offset} }},"
                  f" //asciii 0x{ord(char):04X}\n")

        self.ttf_info_dictionary["nbgl_font_character"].append({
            "encoding": encoding,
            "bitmap_offset": bitmap_offset,
            "width": width,
            "x_min_offset": x_min_offset,
            "y_min_offset": y_min_offset,
            "x_max_offset": x_max_offset,
            "y_max_offset": y_max_offset,
            # Additional fields used for speculos OCR
            "char": ord(char),
            "bitmap_byte_count": bitmap_byte_count
        })

    # -------------------------------------------------------------------------
    def save_nbgl_font_unicode(self, inc, crop, suffix):
        """
        Save the nbgl_unicode_font_t info into the .inc file.
        Structure defined in public_sdk/lib_nbgl/include/nbgl_fonts.h
        typedef struct {
          uint8_t   font_id;            ///< ID of the font, from @ref nbgl_font_id_e
          uint8_t   bpp;                ///< Number of bits/pixels, (nbgl_bpp_t)
          uint8_t   height;             ///< height of all characters in pixels
          uint8_t   line_height;        ///< height of a line for all characters in pixels
          uint8_t   char_kerning;       ///< kerning for the font
          uint8_t   crop;               ///< If false, x_min_offset+y_min_offset=bytes to skip
          uint8_t   y_min;              ///< Most top Y coordinate of any char in the font
          uint8_t   unused[3];          ///< for alignment
          uint16_t  nb_characters;      ///< Number of characters in this font
        } nbgl_font_unicode_t;
        """
        inc.write(
            "\n__attribute__ ((section(\"._nbgl_fonts_\"))) const "
            f"nbgl_font_unicode_t font{self.basename.upper()}{suffix}"
            f"= {{\n"
            f"  {self.font_id_name}, // font id\n"
            f"  (uint8_t) NBGL_BPP_{self.bpp}, // bpp\n"
            f"  {self.real_height}, // height of all characters in pixels\n"
            f"  {self.line_sizes[self.unicode_index]}, // line height in pixels\n"
            f"  {self.kerning}, // kerning in pixels\n"
            f"  {crop}, // crop enabled (1) or not (0)\n"
            f"  {self.char_topmost_y}, // Most top Y coordinate of any char\n"
            "  0,0,0, // for alignment\n"
            f"  {len(self.char_info)}, // Nb of characters\n")
        if not suffix:
            inc.write(f"  characters{self.basename.upper()},\n")
            inc.write(f"  bitmap{self.basename.upper()}\n")

    # -------------------------------------------------------------------------
    def save_nbgl_font_unicode_character(self, inc, char, info):
        """
        Save the nbgl_unicode_font_character_t info into the .inc file, but first
        check that the values do not exceed the boundaries of each fieds.
        Structure defined in public_sdk/lib_nbgl/include/nbgl_fonts.h
        typedef struct {
            uint32_t char_unicode : 21;   ///< plane value from 0 to 16 then 16-bit code.
            uint32_t encoding : 1;        ///< method used to encode bitmap data
            uint32_t width : 6;           ///< width of character in pixels
            uint32_t x_min_offset : 4;    ///< x_min = x_min_offset
            uint32_t y_min_offset : 6;    ///< y_min = (y_min + y_min_offset)
            uint32_t x_max_offset : 4;    ///< x_max = width - x_max_offset
            uint32_t y_max_offset : 6;    ///< y_max = (height - y_max_offset)
            uint32_t bitmap_offset : 15;  ///< offset of this character in chars buffer
            uint32_t over_previous : 1;   ///< flag set to 1 when displayed over previous char
        } nbgl_font_unicode_character_t;
        """
        char_unicode = ord(char)
        bitmap_byte_count = info["size"]
        encoding = info["encoding"]
        bitmap_offset = info["offset"]
        width = info["width"]
        over_previous = info["over_previous"]

        # If it's an empty box, just put everything at 0
        if info["bitmap"] is None or len(info["bitmap"]) == 0:
            x_min_offset = 0
            y_min_offset = 0

            x_max_offset = 0
            y_max_offset = 0
        else:
            x_min_offset = info["left"]
            y_min_offset = info["top"] - self.char_topmost_y

            x_max_offset = width - info["right"]
            # bottom is actually ymax + 1
            y_max_offset = self.real_height - info["bottom"]

        # When crop is False, we may have some bytes to skip at beginning
        if not self.crop:
            skipped_bytes = info["skipped"]
            self.check_max_bits(skipped_bytes, 6, char, "skipped bytes")
            # We'll store 6 bits of skipped bytes in x_min_offset & y_min_offset
            x_min_offset = (skipped_bytes >> 3) & 7
            y_min_offset = skipped_bytes & 7

        # If this character is displayed over previous one, adapt X coordinates
        if over_previous:
            # left_offset is < 0, which can't be stored here => use width to store left_offset
            width = 0 - (info["left_offset"] + info["left"])
            # Combine x_min + x_max to store the real width of displayed data
            real_width = info["right"] - info["left"]
            x_min_offset = real_width // 16
            x_max_offset = real_width % 16

        # Check maximum values
        self.check_max_bits(char_unicode, 21, char, "char_unicode")
        self.check_max_bits(bitmap_byte_count, 15, char, "bitmap_byte_count")
        self.check_max_bits(over_previous, 1, char, "over_previous")
        self.check_max_bits(bitmap_offset, 16, char, "bitmap_offset")
        self.check_max_bits(width, 8, char, "width")
        self.check_max_bits(x_min_offset, 4, char, "x_min_offset")
        self.check_max_bits(y_min_offset, 6, char, "y_min_offset")
        # Next one should never occur, thanks to max_x_max_offset check
        self.check_max_bits(x_max_offset, 4, char, "x_max_offset")
        self.check_max_bits(y_max_offset, 6, char, "y_max_offset")
        self.check_max_bits(encoding, 1, char, "encoding")

        unicode = f"0x{ord(char):06X}"
        self.ttf_info_dictionary["nbgl_font_unicode_character"].append({
            "char_unicode": ord(char),
            "bitmap_byte_count": bitmap_byte_count,
            "bitmap_offset": bitmap_offset,
            "width": width,
            "x_min_offset": x_min_offset,
            "y_min_offset": y_min_offset,
            "x_max_offset": x_max_offset,
            "y_max_offset": y_max_offset,
            "encoding": encoding,
            "over_previous": over_previous,
            # Additional field used for speculos OCR
            "char": ord(char),
        })
        inc.write(f"  {{ 0x{ord(char):06X}, {bitmap_byte_count:3},"
                  f" {bitmap_offset:5}, {encoding:1}, {over_previous:1}, {width:2},"
                  f" {x_min_offset:2}, {y_min_offset:2},"
                  f" {x_max_offset:2}, {y_max_offset:2}}}, "
                  f"//unicode {unicode}\n")

    # -------------------------------------------------------------------------
    def save_bagl_font(self, inc, suffix, first_char, last_char):
        """
        Save the bagl_font_t info into the .inc file.
        Structure defined in public_sdk/lib_bagl/include/bagl.h
        typedef struct {
          uint16_t bitmap_len;      // Size in bytes of all characters bitmaps
          uint8_t font_id;          // to allow for sparse font embedding with a linear enum
          uint8_t bpp;              // for antialiased fonts (blue?)
          uint8_t height;           // Does already contain the nb of skipped lines
          uint8_t baseline;         // Does already contain the nb of skipped lines
          uint8_t first_char;
          uint8_t last_char;
          const bagl_font_character_t * const characters;
          unsigned char const * bitmap; // single bitmap for all chars of a font
        } bagl_font_t;
        """
        height = self.real_height - self.char_topmost_y
        baseline = self.baseline - self.char_topmost_y
        inc.write(
            f"\nconst bagl_font_t font{self.basename.upper()}{suffix}"
            f" = {{\n"
            f"  {self.bitmap_len}, // bitmap len\n"
            f"  {self.font_id_name}, // font id\n"
            f"  {self.bpp}, // bpp => 1 for B&W\n"
            f"  {height}, // height (does already contain the nb of skipped "
            "lines)\n"
            f"  {baseline}, // baseline (does already contain the nb of "
            "skipped lines)\n"
            f"  0x{first_char:X}, // first character\n"
            f"  0x{last_char:X}, // last character\n")
        if not suffix:
            inc.write(f"  characters{self.basename.upper()},\n")
            inc.write(f"  bitmap{self.basename.upper()}\n")

    # -------------------------------------------------------------------------
    def save_bagl_font_character(self, inc, char, info):
        """
        Save the bagl_font_character_t info into the .inc file, but first
        check that the values do not exceed the boundaries of each fieds.
        Structure defined in public_sdk/lib_bagl/include/bagl.h
        typedef struct {
          uint32_t encoding:2;
          uint32_t bitmap_offset:12;
          uint32_t width:5;
          uint32_t x_min_offset:4;
          uint32_t y_min_offset:5;
          uint32_t x_max_offset:4;
        } bagl_font_character_t;
        """
        width = info["width"]

        # If it's an empty box, just put everything at 0
        if info["bitmap"] is None or len(info["bitmap"]) == 0:
            x_min_offset = 0
            y_min_offset = 0
            x_max_offset = 0
            y_max_offset = 0
        else:
            x_min_offset = info["left"]
            x_max_offset = width - info["right"]
            y_min_offset = info["top"] - self.char_topmost_y
            # bottom is actually ymax + 1
            y_max_offset = self.real_height - info["bottom"]

        offset = info["offset"]
        encoding = info["encoding"]

        # Check maximum values
        self.check_max_bits(encoding, 2, char, "encoding")
        self.check_max_bits(offset, 12, char, "bitmap_offset")
        self.check_max_bits(width, 5, char, "width")
        self.check_max_bits(x_min_offset, 4, char, "x_min_offset")
        self.check_max_bits(y_min_offset, 5, char, "y_min_offset")
        self.check_max_bits(x_max_offset, 4, char, "x_max_offset")

        inc.write(f"  {{ {encoding:1}, {offset:5}, {width:2},"
                  f"{x_min_offset}, {y_min_offset}, {x_max_offset} }},"
                  f" //asciii 0x{ord(char):04X}\n")

        self.ttf_info_dictionary["bagl_font_character"].append({
            "encoding": encoding,
            "bitmap_offset": offset,
            "width": width,
            "x_min_offset": x_min_offset,
            "y_min_offset": y_min_offset,
            "x_max_offset": x_max_offset,
            "y_max_offset": y_max_offset,
            # Additional fields used for speculos OCR
            "char": ord(char),
            "bitmap_byte_count": info["size"]
        })

    # -------------------------------------------------------------------------
    def save_bagl_font_unicode(self, inc, suffix):
        """
        Save the bagl_font_unicode_t info into the .inc file.
        Structure defined in public_sdk/lib_bagl/include/bagl.h
        typedef struct {
          uint16_t  bitmap_len;       // Size in bytes of all characters bitmaps
          uint8_t   font_id;
          uint8_t   baseline;         // Does already contain the nb of skipped lines
        #if !defined(HAVE_LANGUAGE_PACK)
          // When using language packs, those 2 pointers does not exists
          const bagl_font_unicode_character_t * const characters;
          unsigned char const * bitmap; // single bitmap for all chars of a font
        #endif //!defined(HAVE_LANGUAGE_PACK)
        } bagl_font_unicode_t;
        """
        baseline = self.baseline - self.char_topmost_y
        inc.write(
            f"\nconst bagl_font_unicode_t font{self.basename.upper()}{suffix}"
            f" = {{\n"
            f"  {self.bitmap_len}, // bitmap len\n"
            f"  {self.font_id_name}, // font id\n"
            f"  {baseline}, // baseline (does already contain the nb of "
            "skipped lines)\n")
        if not suffix:
            inc.write(f"  characters{self.basename.upper()},\n")
            inc.write(f"  bitmap{self.basename.upper()}\n")

    # -------------------------------------------------------------------------
    def save_bagl_font_unicode_character(self, inc, char, info):
        """
        Save the bagl_font_character_t info into the .inc file, but first
        check that the values do not exceed the boundaries of each fieds.
        Structure defined in public_sdk/lib_bagl/include/bagl.h
        typedef struct {
          uint32_t  char_unicode:21;  // plane value from 0 to 16 then 16-bit code.
          uint32_t  width:6;
          uint32_t  x_min_offset:5;   //  x_min = x_min_offset
          uint32_t  y_min_offset:6;   //  Does already contain the nb of skipped lines
          uint32_t  x_max_offset:5;   //  x_max = width - x_max_offset
          uint32_t  encoding:2;       //  method used to encode bitmap data
          uint32_t  bitmap_offset:19;
        } bagl_font_unicode_character_t;
        """
        width = info["width"]

        # If it's an empty box, just put everything at 0
        if info["bitmap"] is None or len(info["bitmap"]) == 0:
            x_min_offset = 0
            y_min_offset = 0
            x_max_offset = 0
        else:
            x_min_offset = info["left"]
            x_max_offset = width - info["right"]
            y_min_offset = info["top"] - self.char_topmost_y

        offset = info["offset"]
        encoding = info["encoding"]

        # Check maximum values
        val = ord(char)
        self.check_max_bits(val, 21, char, "char_unicode")
        self.check_max_bits(width, 6, char, "width")
        self.check_max_bits(x_min_offset, 5, char, "x_min_offset")
        self.check_max_bits(y_min_offset, 6, char, "y_min_offset")
        self.check_max_bits(x_max_offset, 5, char, "x_max_offset")
        self.check_max_bits(encoding, 2, char, "encoding")
        self.check_max_bits(offset, 19, char, "bitmap_offset")

        unicode = f"0x{ord(char):06X}"
        self.ttf_info_dictionary["bagl_font_unicode_character"].append({
            "char_unicode": ord(char),
            "width": width,
            "x_min_offset": x_min_offset,
            "x_max_offset": x_max_offset,
            "y_min_offset": y_min_offset,
            "encoding": encoding,
            "bitmap_offset": offset,
            # Additional fields used for speculos OCR
            "char": ord(char),
            "bitmap_byte_count": info["size"]
        })

        inc.write(f"  {{ 0x{ord(char):06X}, {width:3}, "
                  f"{x_min_offset:2}, {y_min_offset:2},"
                  f"{x_max_offset:2}, {encoding:1}, "
                  f"{offset:5} }}, //unicode {unicode}\n")

    # -------------------------------------------------------------------------
    @staticmethod
    def swap32(data):
        """
        Transform a Big Endian value into Little Endian one, and vice versa.
        """
        if isinstance(data, bytes):
            return [struct.unpack("<I", struct.pack(">I", i))[0] for i in data]

        result = (data & 0x000000FF) << 24
        result += (data & 0x0000FF00) << 8
        result += (data & 0x00FF0000) >> 8
        result += (data >> 24) & 0xFF
        return result

    # -------------------------------------------------------------------------
    def get_font_id(self):
        """
        Return the font id.
        """
        # Those ids are defined in bagl.h file, in bagl_font_id_e enums.
        bagl_font_ids = {
            "BAGL_FONT_LUCIDA_CONSOLE_8PX": 0,
            "BAGL_FONT_OPEN_SANS_LIGHT_16_22PX": 1,
            "BAGL_FONT_OPEN_SANS_REGULAR_8_11PX": 2,
            "BAGL_FONT_OPEN_SANS_REGULAR_10_13PX": 3,
            "BAGL_FONT_OPEN_SANS_REGULAR_11_14PX": 4,
            "BAGL_FONT_OPEN_SANS_REGULAR_13_18PX": 5,
            "BAGL_FONT_OPEN_SANS_REGULAR_22_30PX": 6,
            "BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX": 7,
            "BAGL_FONT_OPEN_SANS_EXTRABOLD_11px": 8,
            "BAGL_FONT_OPEN_SANS_LIGHT_16px": 9,
            "BAGL_FONT_OPEN_SANS_REGULAR_11px": 10,
            "BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX": 11,
            "BAGL_FONT_OPEN_SANS_SEMIBOLD_11_16PX": 12,
            "BAGL_FONT_OPEN_SANS_SEMIBOLD_13_18PX": 13,
            "BAGL_FONT_SYMBOLS_0": 14,
            "BAGL_FONT_SYMBOLS_1": 15
        }
        # Those ids are defined in nbgl_fonts.h file, in nbgl_font_id_e enums.
        nbgl_font_ids = {
            "BAGL_FONT_INTER_REGULAR_24px": 0,
            "BAGL_FONT_INTER_SEMIBOLD_24px": 1,
            "BAGL_FONT_INTER_MEDIUM_32px": 2,
            "BAGL_FONT_INTER_REGULAR_24px_1bpp": 3,
            "BAGL_FONT_INTER_SEMIBOLD_24px_1bpp": 4,
            "BAGL_FONT_INTER_MEDIUM_32px_1bpp": 5,
            "BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp": 8,
            "BAGL_FONT_OPEN_SANS_LIGHT_16px_1bpp": 9,
            "BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp": 10,
            "BAGL_FONT_INTER_REGULAR_28px": 11,
            "BAGL_FONT_INTER_SEMIBOLD_28px": 12,
            "BAGL_FONT_INTER_MEDIUM_36px": 13,
            "BAGL_FONT_INTER_REGULAR_28px_1bpp": 14,
            "BAGL_FONT_INTER_SEMIBOLD_28px_1bpp": 15,
            "BAGL_FONT_INTER_MEDIUM_36px_1bpp": 16,
            "BAGL_FONT_NANOTEXT_MEDIUM_18px_1bpp": 17,
            "BAGL_FONT_NANOTEXT_BOLD_18px_1bpp": 18,
            "BAGL_FONT_NANODISPLAY_SEMIBOLD_24px_1bpp": 19
        }
        if self.nbgl:
            font_ids = nbgl_font_ids
            font_id = 0        # BAGL_FONT_INTER_REGULAR_24px by default
        else:
            font_ids = bagl_font_ids
            font_id = 10        # BAGL_FONT_OPEN_SANS_REGULAR_11px by default

        if self.font_id_name in font_ids.keys():
            font_id = font_ids[self.font_id_name]

        # Set the nanos flag if we are using an LNX/LNS+ font
        if font_id >= 8 and font_id <= 10:
            self.nanos = True

        # Set the apex flag if we are using an Apex font
        if font_id >= 17:
            self.apex = True

        return font_id

    # -------------------------------------------------------------------------
    def read_ids(self, id_filename):
        """
        Read a text file (a .h actually) containing all IDs to use.
        """
        if not os.path.exists(id_filename):
            sys.stderr.write(f"Can't open file {id_filename}\n")
            sys.exit(-5)

        # Read the .h file and store each ID found
        with open(id_filename, "r", encoding="utf-8", errors='ignore') as id_file:
            for line in id_file:
                # Ignore lines that don't start with "
                if line[0] != '"':
                    continue
                # Remove leading "
                line = line.lstrip('"')
                # Remove trailing ",
                line = line.rstrip('",\n')
                self.ids.append(line)

    # -------------------------------------------------------------------------
    def add_unicode_chars(self, string, string_id):
        """
        Parse string and add unicode characters not already stored
        """
        # If we have a list of IDs, be sure that one is included in it
        if len(self.ids) != 0:
            if string_id not in self.ids:
                return

        for char in string:
            # Just check unicode characters
            # \b, \n, \f and \e\xAB will appear individually, as ASCII chars!
            if ord(char) > 0x7F and char not in self.unicode_chars:
                self.unicode_chars += char

    # -------------------------------------------------------------------------
    def parse_json_strings(self, filenames):
        """
        Parse the provided JSON file(s) and scan all unicode chars found in strings
        """
        for file in filenames.split():
            if not os.path.exists(file):
                sys.stderr.write(f"Can't open file {file}\n")
                sys.exit(-4)

            # Read the JSON file into json_data
            with open(file, "r", encoding="utf-8", errors='ignore') as json_file:
                json_data = json.load(json_file, strict=False)

            # Parse all strings contained in txt fields of json_data
            for category in json_data:
                # Skip Smartling & branch related parts of the JSON file
                if category.lower() == "smartling" or category.lower() == "branch":
                    continue

                # Parse all "text" strings and add the unicode characters
                for string in json_data[category]:
                    if "text" not in string.keys() or "id" not in string.keys():
                        sys.stdout.write(f"Skipping {string} because it does "
                                         "not contain \"text\" or \"id\"!\n")
                        continue
                    self.add_unicode_chars(string['text'], string['id'])

        return self.unicode_chars


# -----------------------------------------------------------------------------
def change_ext(filename, extension):
    """
    Change a filename extension
    """
    # Get the filename without current extension
    new_filename = os.path.splitext(filename)[0]
    # Add the new extension
    new_filename += extension

    return new_filename


# -----------------------------------------------------------------------------
# Program entry point:
# -----------------------------------------------------------------------------
if __name__ == "__main__":

    # -------------------------------------------------------------------------
    def main(args):
        """
        Main entry point
        """

        with TTF2INC(args) as ttf:

            # If we provided a list of IDs to use, read the file!
            if args.id:
                ttf.read_ids(args.id)

            # if JSON filename(s) was provided, parse them to get the strings
            if args.json_filenames:
                string = ttf.parse_json_strings(args.json_filenames)
            # else use the provided string or generate all wanted ASCII characters:
            else:
                string = args.string
            if len(string) == 0 or args.ascii:
                for value in range(ttf.first_character, ttf.last_character+1):
                    string += chr(value)

            # Check if string contains only ASCII characters or have UNICODE ondes
            for char in string:
                if ord(char) > 0x7F:
                    ttf.unicode_needed = True
                    break
            # Now, we know if we will generate ASCII or UNICODE data

            # Find dimension used by specified characters with that font
            # (also, find which font is really the one to use here)
            ttf.find_string_dimension(string)

            # We now know everything on this font & used chars => build filenames
            ttf.build_names()

            # Now, handle all characters from the string:
            for char in string:
                ttf.update_font_info(ttf.char_info[char]['index'])
                try:
                    # We could have done everything in one step, from .ttf to
                    # .inc, but to allow manual editing of generated images
                    # files, we will process chars in two steps:
                    # - Step 1: from .ttf (or .otf) to .png (or .bmp)
                    # - Step 2: from .png (or .bmp) to .inc

                    # STEP 1: Generate the img if it doesn't exist
                    # (or load it otherwise)
                    ttf.get_char_image(char)

                    # STEP 2: Get bitmap data based on img content
                    ttf.get_char_bitmap(char)

                except (BaseException, Exception) as error:
                    sys.stderr.write(f"An error occurred while processing char"
                                     f" '{char}' with font {ttf.name}"
                                     f" size {ttf.font_size}: {error}\n")
                    return 1

            if args.verbose:
                sys.stdout.write(f"Font {ttf.basename.upper()}: "
                                 f"ascent={ttf.ascent}, descent={ttf.descent}"
                                 f", font size={ttf.font.size}, "
                                 f"font height={ttf.font_heights[0]}\n"
                                 f"crop={ttf.crops[0]}, "
                                 f"char_leftmost_x={ttf.char_leftmost_x}, "
                                 f"char_topmost_y={ttf.char_topmost_y}, "
                                 f"real font height={ttf.real_height}, "
                                 f"line_height={ttf.line_sizes[ttf.unicode_index]}, "
                                 f"baseline={ttf.baselines[ttf.unicode_index]}\n")

            if ttf.real_height > ttf.line_sizes[ttf.unicode_index]:
                # If baseline_offset is > 0, it means some chars are displayed at Y < 0
                # => the only solution is to increase line_height value!
                if ttf.baseline_offset > 0:
                    sys.stderr.write(f"WARNING: Font {ttf.font_id_name} have "
                                     f"a real font height value ({ttf.real_height}) > "
                                     f"to line_height value ({ttf.line_sizes[ttf.unicode_index]}) "
                                     f"=> as 'baseline_offset' value ({ttf.baseline_offset}) "
                                     f"is already > 0, you MUST increase line_height value!\n")
                else:
                    delta = ttf.real_height - ttf.line_sizes[ttf.unicode_index]
                    sys.stderr.write(f"WARNING: Font {ttf.font_id_name} have "
                                     f"a real font height value ({ttf.real_height}) > "
                                     f"to line_height value ({ttf.line_sizes[ttf.unicode_index]}) "
                                     f"=> 'baseline_offset' value should be set to {-(delta)} !\n")

            # Last step: generate .inc & json files with data for all chars
            # (the .inc file will be stored in src directory)
            if args.output_name:
                inc_filename = args.output_name
            else:
                filename = f"{ttf.font_prefix}{ttf.basename}.inc"
                inc_filename = os.path.join("../../../public_sdk/lib_bagl/src/", filename)

            # Force .inc extension for inc_filename
            inc_filename = change_ext(inc_filename, ".inc")

            # Build the corresponding .json file
            inc_json = change_ext(inc_filename, ".json")

            if args.suffix:
                suffix = args.suffix
            else:
                suffix = ""
            if args.verbose:
                sys.stdout.write(f"Generating file {inc_filename}\n")

            ttf_info_list = []
            if args.append:
                mode = "a"
                # Read previous entries, if such file exists
                if inc_json and os.path.exists(inc_json):
                    with open(inc_json, "r", encoding="utf-8", errors='ignore') as json_file:
                        ttf_info_list = json.load(json_file, strict=False)
            else:
                mode = "w"

            ttf.ttf_info_dictionary = {}
            crop = 0
            if ttf.crop:
                crop = 1
            with open(inc_filename, mode, encoding="utf-8") as inc:
                if not args.output_name:
                    inc.write("/* @BANNER@ */\n\n")
                # Write the array containing all bitmaps:
                if ttf.nbgl:
                    inc.write('#include "nbgl_fonts.h"\n\n')
                    inc.write(
                        f"__attribute__ ((section(\"._nbgl_fonts_\"))) "
                        f"const unsigned char bitmap{ttf.basename.upper()}"
                        f"{suffix}[] = {{\n")
                else:
                    inc.write(
                        f"const unsigned char bitmap{ttf.basename.upper()}"
                        f"{suffix}[] = {{\n")
                offset = 0
                first_char = None
                ttf.ttf_info_dictionary["bitmap"] = bytes()
                # Write the bitmap part of each character
                for char, info in sorted(ttf.char_info.items()):
                    image_data = info["bitmap"]
                    if image_data is not None:
                        ttf.ttf_info_dictionary["bitmap"] += image_data
                    info["offset"] = offset
                    offset += info["size"]
                    ttf.bitmap_len = offset

                    # Keep track of first and last chars:
                    if not first_char:
                        first_char = ord(char)
                    last_char = ord(char)

                    if ttf.unicode_needed:
                        # Get the unicode value of that char:
                        # (plane value from 0 to 16 then a 16-bit code)
                        inc.write(f"//unicode 0x{ord(char):06X}\n")
                    else:
                        inc.write(f"//ascii 0x{ord(char):04X}\n")
                    for i in range(0, info["size"], 8):
                        inc.write("  " + ", ".join("0x{0:02X}".format(c) for c
                                                   in image_data[i:i+8]) + ",")
                        inc.write("\n")

                inc.write("};\n")

                # Serialize bitmap
                ttf.ttf_info_dictionary["bitmap"] = base64.b64encode(
                    ttf.ttf_info_dictionary["bitmap"]).decode('utf-8')

                # Write the array containing information about characters:
                if ttf.unicode_needed:
                    typedef = f"{ttf.font_prefix}unicode_character_t"
                    ttf.ttf_info_dictionary[f"{ttf.font_prefix}unicode_character"] = []
                else:
                    typedef = f"{ttf.font_prefix}character_t"
                    ttf.ttf_info_dictionary[f"{ttf.font_prefix}character"] = []

                inc.write("\n")
                if ttf.nbgl:
                    inc.write(" __attribute__ ((section(\"._nbgl_fonts_\"))) ")
                inc.write(
                    f"const {typedef} characters{ttf.basename.upper()}"
                    f"{suffix}[{len(ttf.char_info)}] = {{\n")

                # Save each character info into the .inc file
                for char, info in sorted(ttf.char_info.items()):

                    if ttf.unicode_needed:
                        if ttf.nbgl:
                            ttf.save_nbgl_font_unicode_character(
                                inc, char, info)
                        else:
                            ttf.save_bagl_font_unicode_character(
                                inc, char, info)
                    else:
                        if ttf.nbgl:
                            ttf.save_nbgl_font_character(
                                inc, char, info)
                        else:
                            ttf.save_bagl_font_character(
                                inc, char, info)

                inc.write("};\n")

                # Write the struct containing information about the font:
                if ttf.unicode_needed:
                    typedef = f"{ttf.font_prefix}unicode_t"
                    ttf.ttf_info_dictionary[f"{ttf.font_prefix}unicode"] = {
                        "bitmap_len": ttf.bitmap_len,
                        "font_id": ttf.get_font_id(),
                        "bpp": ttf.bpp,
                        "height": ttf.real_height,
                        "baseline": ttf.baseline,
                        "line_height": ttf.line_sizes[ttf.unicode_index],
                        "char_kerning": ttf.kerning,
                        "crop": crop,
                        "nb_characters": len(ttf.char_info),
                        "char_leftmost_x": ttf.char_leftmost_x,
                        "char_topmost_y": ttf.char_topmost_y
                    }
                else:
                    typedef = f"{ttf.font_prefix}t"

                if ttf.unicode_needed:
                    if ttf.nbgl:
                        ttf.save_nbgl_font_unicode(
                            inc, crop, suffix)
                    else:
                        ttf.save_bagl_font_unicode(
                            inc, suffix)
                else:
                    if ttf.nbgl:
                        ttf.save_nbgl_font(
                            inc, crop, suffix, first_char, last_char)
                    else:
                        ttf.save_bagl_font(
                            inc, suffix, first_char, last_char)

                inc.write("};\n")

            # Generate a JSON file with all font related info?
            ttf_info_list.append(ttf.ttf_info_dictionary)
            with open(inc_json, "w", encoding="utf-8") as json_file:
                json.dump(ttf_info_list, json_file, indent=2)
                # Be sure there is a newline at the end of the file
                json_file.write("\n")

        return 0

    # -------------------------------------------------------------------------
    # Parse arguments:
    parser = argparse.ArgumentParser(
        description="Convert a .ttf file into a .inc file (Build #251030.1451)")

    parser.add_argument(
        "--ascii",
        dest="ascii", action='store_true',
        default=False,
        help="Add ASCII characters ('%(default)s' by default)")

    parser.add_argument(
        "-n", "--name", "--font_name",
        dest="font_name", type=str,
        default=TTF2INC.FONT_NAME,
        help="Font name ('%(default)s' by default)")

    parser.add_argument(
        "-b", "--bpp",
        dest="bpp", type=int,
        default=1,
        help="Bits Per Point (bpp) to use ('%(default)s' by default)")

    parser.add_argument(
        "--font_id_name",
        dest="font_id_name", type=str,
        default=None,
        help="Font ID name ('%(default)s' by default)")

    parser.add_argument(
        "--output",
        dest="output_name", type=str,
        default=None,
        help="Output name ('%(default)s' by default)")

    parser.add_argument(
        "--suffix",
        dest="suffix", type=str,
        default=None,
        help="Suffix added to variable names ('%(default)s' by default)")

    parser.add_argument(
        "-s", "--size", "--font_size",
        dest="font_size", type=int,
        default=TTF2INC.FONT_SIZE,
        help="Font size in pixels ('%(default)s' by default)")

    parser.add_argument(
        "--line_size",
        dest="line_size", type=int,
        default=TTF2INC.LINE_SIZE,
        help="Line size in pixels ('%(default)s' by default)")

    parser.add_argument(
        "-f", "--first", "--first_character",
        dest="first_character", type=int,
        default=TTF2INC.FIRST_CHARACTER,
        help="First character ('%(default)s' by default)")

    parser.add_argument(
        "-l", "--last", "--last_character",
        dest="last_character", type=int,
        default=TTF2INC.LAST_CHARACTER,
        help="Last character ('%(default)s' by default)")

    parser.add_argument(
        "-c", "--chars", "--string",
        dest="string", type=str,
        default=TTF2INC.STRING,
        help="String with characters to convert ('%(default)s' by default)")

    parser.add_argument(
        "-i", "--init",
        dest="init_file", type=str,
        help="init file for this font/size")

    parser.add_argument(
        "-j", "--json",
        dest="json_filenames", type=str,
        default=None,
        help="Full path of JSON filenames containing all strings ('%(default)s' by default)")

    parser.add_argument(
        "--id",
        dest="id", type=str,
        default=None,
        help="Filename containing all IDs to use ('%(default)s' by default)")

    parser.add_argument(
        "-a", "--append",
        dest="append", action='store_true',
        default=False,
        help="Append to existing INC file(s) ('%(default)s' by default)")

    parser.add_argument(
        "-o", "--overwrite",
        dest="overwrite", action='store_true',
        default=False,
        help="Overwrite & ignore existing picture(s) ('%(default)s' by default)")

    parser.add_argument(
        "--save",
        dest="save", action='store_true',
        default=False,
        help="Save PNG file(s) ('%(default)s' by default)")

    parser.add_argument(
        "-v", "--verbose",
        dest="verbose", action='store_true',
        default=False,
        help="Add verbosity to output ('%(default)s' by default)")

    # Call main function:
    EXIT_CODE = main(parser.parse_args())

    sys.exit(EXIT_CODE)
