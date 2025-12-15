#!/usr/bin/env python3
# coding: utf-8
# -----------------------------------------------------------------------------
"""
This module contain tools to test different custom RLE coding/decoding.
"""
# -----------------------------------------------------------------------------
import argparse
import base64
import json
import os
import sys

# -----------------------------------------------------------------------------
# Regular RLE encoding
# -----------------------------------------------------------------------------
class RLECustomBase:
    """
    Class handling regular RLE encoding (1st pass)
    """
    CMD_FILL                    = 0
    CMD_COPY                    = 1
    CMD_FILL_WHITE              = 2
    CMD_FILL_BLACK              = 3
    CMD_SIMPLE_PATTERN_WHITE    = 4
    CMD_SIMPLE_PATTERN_BLACK    = 5
    CMD_DOUBLE_PATTERN_WHITE    = 6
    CMD_DOUBLE_PATTERN_BLACK    = 7
    CMD_TRIPLE_PATTERN_WHITE    = 8
    CMD_TRIPLE_PATTERN_BLACK    = 9
    CMD_SKIP                    = 10

    double_pattern_black = []
    double_pattern_white = []

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__()

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        self.encoded = None

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
    def remove_duplicates(self, pairs):
        """
        Check if there are some duplicated pairs (same values) and merge them.
        """
        self.bpp = self.bpp

        index = len(pairs) - 1
        while index >= 1:
            repeat1, value1 = pairs[index-1]
            repeat2, value2 = pairs[index]
            # Do we have identical consecutives values?
            if value1 == value2:
                repeat1 += repeat2
                # Merge them and remove last entry
                pairs[index-1] = (repeat1, value1)
                pairs.pop(index)
            index -= 1

        return pairs

    # -------------------------------------------------------------------------
    def bpp4_to_values(self, data):
        """
        Expand each bytes of data into 2 quartets.
        Return an array of values (from 0x00 to 0x0F)
        """
        self.bpp = self.bpp

        output = []
        for byte in data:
            lsb_bpp4 = byte & 0x0F
            byte >>= 4
            output.append(byte & 0x0F)
            output.append(lsb_bpp4)

        return output

    # -------------------------------------------------------------------------
    def values_to_bpp4(self, data):
        """
        Takes values (assumed from 0x00 to 0x0F) in data and returns an array
        of bytes containing quartets with values concatenated.
        """
        self.bpp = self.bpp

        output = bytes()
        remaining_values = len(data)
        index = 0
        while remaining_values > 1:
            byte = data[index] & 0x0F
            index += 1
            byte <<= 4
            byte |= data[index] & 0x0F
            index += 1
            remaining_values -= 2
            output += bytes([byte])

        # Is there a remaining quartet ?
        if remaining_values != 0:
            byte = data[index] & 0x0F
            byte <<= 4  # Store it in the MSB
            output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def bpp1_to_values(self, data):
        """
        Expand each bytes of data into 8 bits, each stored in a byte
        Return an array of values (containing bytes values 0 or 1)
        """
        self.bpp = self.bpp

        output = []
        for byte in data:
            # first pixel is in bit 10000000
            for shift in range(7, -1, -1):
                pixel = byte >> shift
                pixel &= 1
                output.append(pixel)

        assert len(output) == (len(data)*8)

        return output

    # -------------------------------------------------------------------------
    def values_to_bpp1(self, data):
        """
        Takes values (bytes containing 0 or 1) in data and returns an array
        of bytes containing bits concatenated.
        (first pixel is bit 10000000 of first byte)
        """
        self.bpp = self.bpp

        output = bytes()
        remaining_values = len(data)
        index = 0
        bits = 7
        byte = 0
        while remaining_values > 0:
            pixel = data[index] & 1
            index += 1
            byte |= pixel << bits
            bits -= 1
            if bits < 0:
                # We read 8 pixels: store them and get ready for next ones
                output += bytes([byte])
                bits = 7
                byte = 0
            remaining_values -= 1

        # Is there some remaining pixels stored?
        if bits != 7:
            output += bytes([byte])

        nb_bytes = len(data)/8
        if len(data) % 8:
            nb_bytes += 1
        assert len(output) == nb_bytes

        return output

    # -------------------------------------------------------------------------
    def bpp_to_values(self, data):
        """
        Expand each bytes of data, containing pixels (4BPP or 1BPP)
        Return an array of values (0x00 to 0x0F or 0x00 to 0x01)
        """
        if self.bpp == 4:
            return self.bpp4_to_values(data)

        return self.bpp1_to_values(data)

    # -------------------------------------------------------------------------
    def values_to_bpp(self, data):
        """
        Takes values (0x00 to 0x0F or 0x00 to 0x01) in data and returns an
        array of bytes containing pixels concatenated.
        """
        if self.bpp == 4:
            return self.values_to_bpp4(data)

        return self.values_to_bpp1(data)

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=128):
        """
        Encode tuples containing (repeat, val) into packed values.
        (empty method intended to be overwritten)
        """
        if self.bpp in (1, 4) or max_count:
            pass

        return pairs

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        (empty method intended to be overwritten)
        """
        self.bpp = self.bpp

        return data

    # -------------------------------------------------------------------------
    def encode_pass1(self, data):
        """
        Encode array of values using 'normal' RLE.
        Return an array of tuples containing (repeat, val)
        """
        output = []
        previous_value = -1
        count = 0
        for value in data:
            # Same value than before?
            if value == previous_value:
                count += 1
            else:
                # Store previous result
                if count:
                    pair = (count, previous_value)
                    output.append(pair)
                # Start a new count
                previous_value = value
                count = 1

        # Store previous result
        if count:
            pair = (count, previous_value)
            output.append(pair)

        if self.verbose and False:
            sys.stdout.write(f"Nb values on pass1: {len(output)}\n")
            for repeat, value in output:
                sys.stdout.write(f"{repeat:3d} x 0x{value:02X}\n")

        return output

    # -------------------------------------------------------------------------
    def decode_pass1(self, data):
        """
        Decode array of tuples containing (repeat, val).
        Return an array of values.
        """
        self.bpp = self.bpp

        output = []
        for repeat, value in data:
            for _ in range(repeat):
                output.append(value)

        return output

    # -------------------------------------------------------------------------
    def encode(self, data):
        """
        Input: array of packed pixels
        - convert to an array of values
        - convert to tuples of (repeat, value)
        - encode using custom RLE
        Output: array of compressed bytes
        """
        values = self.bpp_to_values(data)
        pairs = self.encode_pass1(values)

        # Second pass
        self.encoded = self.encode_pass2(pairs)

        # Decode to check everything is fine
        decompressed = self.decode(self.encoded)
        if decompressed != data:
            # This will never happen in prod environment, only when testing
            sys.stdout.write("Encoding/Decoding is WRONG!!!\n")
            sys.exit(111)

        return self.encoded

    # -------------------------------------------------------------------------
    def decode(self, data):
        """
        Input: array of compressed bytes
        - decode to pairs using custom RLE
        - convert the tuples of (repeat, value) to values
        - convert to an array of packed pixels
        Output: array of packed pixels
        """
        pairs = self.decode_pass2(data)
        values = self.decode_pass1(pairs)

        decoded = self.values_to_bpp(values)

        return decoded

    # -------------------------------------------------------------------------
    def get_encoded_size(self, data):
        """
        Return the encoded size.
        (to be overwritten if something more complex than len() need to be done)
        """
        self.bpp = self.bpp

        return len(data)

# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustom1 (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain:
    * for 1 BPP: RRRRRRRV
      - RRRRRRR: repeat count - 1 => allow to store 1 to 256 repeat counts
      - V: value of the 1BPP pixel (0 or 1)
    * for 4 BPP: RRRRVVVV
    - RRRR: repeat count - 1 => allow to store 1 to 16 repeat counts
    - VVVV: value of the 4BPP pixel
    """
    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=16):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated bytes will contain:
        RRRRVVVV
        - RRRR: repeat count - 1 => allow to store 1 to 16 repeat counts
        - VVVV: value of the 4BPP pixel
        """
        self.bpp = self.bpp

        output = bytes()
        for repeat, value in pairs:
            # We can't store more than a repeat count of max_count
            while repeat > max_count:
                count = max_count - 1
                byte = count << 4
                byte |= value & 0x0F
                output += bytes([byte])
                repeat -= max_count
            if repeat:
                count = repeat - 1
                byte = count << 4
                byte |= value & 0x0F
                output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        RRRRVVVV
        - RRRR: repeat count - 1 => allow to store 1 to 16 repeat counts
        - VVVV: value of the 4BPP pixel
        """
        pairs = []
        for byte in data:
            value = byte & 0x0F
            byte >>= 4
            byte &= 0x0F
            repeat = 1 + byte
            pairs.append((repeat, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs

# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte +
# - white handling
# -----------------------------------------------------------------------------
class RLECustom2 (RLECustom1):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain:
    1XXXXXXX
    0XXXXXXX
    With:
    * 1RRRRRRR
        - RRRRRRRR is repeat count - 1 of White (0xF) quartets
    * 0RRRVVVV
        - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
        - VVVV: value of the 4BPP pixel
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=128):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated bytes will contain:
        1XXXXXXX
        0XXXXXXX
        With:
          * 1RRRRRRR
            - RRRRRRRR is repeat count - 1 of White (0xF) quartets (max=127+1)
          * 0RRRVVVV
            - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
            - VVVV: value of the 4BPP pixel
        """
        self.bpp = self.bpp

        output = bytes()
        for repeat, value in pairs:
            # Handle white
            if value == 0x0F:
                # We can't store more than a repeat count of 128
                while repeat > max_count:
                    byte = 0x80
                    byte |= max_count - 1
                    output += bytes([byte])
                    repeat -= max_count
                if repeat:
                    byte = 0x80
                    byte |= repeat - 1
                    output += bytes([byte])
            else:
                # We can't store more than a repeat count of 8
                while repeat > 8:
                    count = 8 - 1
                    byte = count << 4
                    byte |= value & 0x0F
                    output += bytes([byte])
                    repeat -= 8
                if repeat:
                    count = repeat - 1
                    byte = count << 4
                    byte |= value & 0x0F
                    output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        1XXXXXXX
        0XXXXXXX
        With:
          * 1RRRRRRR
            - RRRRRRRR is repeat count - 1 of White (0xF) quartets
          * 0RRRVVVV
            - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
            - VVVV: value of the 4BPP pixel
        """
        pairs = []
        for byte in data:
            # Is it a big duplication of whites ?
            if byte & 0x80:
                byte &= 0x7F
                repeat = 1 + byte
                value = 0x0F
            else:
                value = byte & 0x0F
                byte >>= 4
                byte &= 0x07
                repeat = 1 + byte
            pairs.append((repeat, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs

# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte +
# - white handling
# - copy range of bytes
# -----------------------------------------------------------------------------
class RLECustom3 (RLECustom2):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain:
    11RRRRRR
    10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
    0RRRVVVV
    With:
    * 11RRRRRR
        - RRRRRRR is repeat count - 1 of White (0xF) quartets (max=63+1)
    * 10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
        - RR is repeat count - 3 of quartets (max=3+3 => 6 quartets)
        - VVVV: value of 1st 4BPP pixel
        - WWWW: value of 2nd 4BPP pixel
        - XXXX: value of 3rd 4BPP pixel
        - YYYY: value of 4th 4BPP pixel
        - ZZZZ: value of 5th 4BPP pixel
        - QQQQ: value of 6th 4BPP pixel
    * 0RRRVVVV
        - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
        - VVVV: value of the 4BPP pixel
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=64):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated bytes will contain:
        11RRRRRR
        10RRVVVV WWWWXXXX YYYYZZZZ
        0RRRVVVV
        With:
          * 11RRRRRR
            - RRRRRRR is repeat count - 1 of White (0xF) quartets (max=63+1)
          * 10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
            - RR is repeat count - 3 of quartets (max=3+3 => 6 quartets)
            - VVVV: value of 1st 4BPP pixel
            - WWWW: value of 2nd 4BPP pixel
            - XXXX: value of 3rd 4BPP pixel
            - YYYY: value of 4th 4BPP pixel
            - ZZZZ: value of 5th 4BPP pixel
            - QQQQ: value of 6th 4BPP pixel
          * 0RRRVVVV
            - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
            - VVVV: value of the 4BPP pixel
        """
        self.bpp = self.bpp

        # First, generate data in 10RRRRRR/0RRRVVVV format
        single_output = super().encode_pass2(pairs, max_count)

        # Now, parse array to find consecutives singles (0000VVVV)
        output = bytes()
        index = 0
        while index < len(single_output):
            # Do we have some 'singles'?
            byte = single_output[index]
            if (byte & 0xF0) != 0x00:
                # No, just store it
                if byte & 0x80:
                    byte |= 0x40    # 11RRRRRR
                output += bytes([byte])
                index += 1
            else:
                # Check how many 'singles' we have
                count = 1
                while ((index+count) < len(single_output)) and \
                      (single_output[index+count] & 0xF0) == 0x00:
                    count += 1
                # No more than 6 singles
                if count > 6:
                    # Special case: if count = 8 then do 5+3
                    if count == 8:
                        count = 5 # to allow storing next 3 singles!!
                    else:
                        count = 6
                # Do we have at least 3 singles?
                if count >= 3:
                    # Store 10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
                    # 3 singles => 1 + 1 bytes
                    # 4 singles => 1 + 2 bytes (1 quartet unused)
                    # 5 singles => 1 + 2 bytes
                    # 6 singles => 1 + 3 bytes (1 quartet unused)
                    msb_byte = count - 3
                    msb_byte <<= 4
                    msb_byte |= 0x80
                    byte |= msb_byte
                    # Store first byte
                    output += bytes([byte])
                    index += 1
                    count -= 1
                    while count > 0:
                        byte = single_output[index] # No need to mask
                        index += 1
                        count -= 1
                        byte <<= 4
                        # Do we have an other quartet?
                        if count > 0:
                            byte |= single_output[index] # No need to mask
                            index += 1
                            count -= 1
                        # Store the quartet(s)
                        output += bytes([byte])
                else:
                    # No, just store that single
                    output += bytes([byte])
                    index += 1

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        11RRRRRR
        10RRVVVV WWWWXXXX YYYYZZZZ
        0RRRVVVV
        With:
          * 11RRRRRR
            - RRRRRRR is repeat count - 1 of White (0xF) quartets (max=63+1)
          * 10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
            - RR is repeat count - 3 of quartets (max=3+3 => 6 quartets)
            - VVVV: value of 1st 4BPP pixel
            - WWWW: value of 2nd 4BPP pixel
            - XXXX: value of 3rd 4BPP pixel
            - YYYY: value of 4th 4BPP pixel
            - ZZZZ: value of 5th 4BPP pixel
            - QQQQ: value of 6th 4BPP pixel
          * 0RRRVVVV
            - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
            - VVVV: value of the 4BPP pixel
        """
        pairs = []
        index = 0
        while index < len(data):
            byte = data[index]
            index += 1
            # Is it a big duplication of whites or singles?
            if byte & 0x80:
                # Is it a big duplication of whites?
                if byte & 0x40:
                    # 11RRRRRR
                    byte &= 0x3F
                    repeat = 1 + byte
                    value = 0x0F
                # We need to decode singles
                else:
                    # 10RRVVVV WWWWXXXX YYYYZZZZ
                    count = byte & 0x30
                    count >>= 4
                    count += 3
                    value = byte & 0x0F
                    pairs.append((1, value))
                    count -= 1
                    while count > 0 and index < len(data):
                        byte = data[index]
                        index += 1
                        value = byte >> 4
                        value &= 0x0F
                        pairs.append((1, value))
                        count -= 1
                        if count > 0:
                            value = byte & 0x0F
                            pairs.append((1, value))
                            count -= 1
                    continue
            else:
                # 0RRRVVVV
                value = byte & 0x0F
                byte >>= 4
                byte &= 0x07
                repeat = 1 + byte

            pairs.append((repeat, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs

# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustom4 (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain:
    RRRRRRRV
    - RRRRRRRRR: repeat count - 1 => allow to store 1 to 128 repeat counts
    - V: value of the 1BPP pixel
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=128):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated bytes will contain:
        RRRRRRRV
        - RRRRRRRRR: repeat count - 1 => allow to store 1 to 128 repeat counts
        - V: value of the 1BPP pixel
        """
        self.bpp = self.bpp

        output = bytes()
        for repeat, value in pairs:
            # We can't store more than a repeat count of max_count
            while repeat > max_count:
                count = max_count - 1
                byte = count << 1
                byte |= value & 1
                output += bytes([byte])
                repeat -= max_count
            if repeat:
                count = repeat - 1
                byte = count << 1
                byte |= value & 1
                output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        RRRRRRRV
        - RRRRRRRRR: repeat count - 1 => allow to store 1 to 128 repeat counts
        - V: value of the 1BPP pixel
        """
        pairs = []
        for byte in data:
            value = byte & 1
            byte >>= 1
            byte &= 0x7F
            repeat = 1 + byte
            pairs.append((repeat, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustomN (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain triplets with:
    - Command (Fill or Copy)
    - Repeat count
    - Value: value(s) to be Filled/Copied
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        self.commands = []
        self.repeat = []
        self.values = []

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=64):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated packed values will contain:
        - Command (Fill or Copy)
        - Repeat count
        - Value: value(s) to be Filled/Copied
        """
        infos = []
        index = 0
        if self.bpp == 1:
            #threshold = 7
            threshold = 2
        else:
            threshold = 3

        while index < len(pairs):
            repeat, value = pairs[index]
            index += 1
            # We can't store more than a repeat count of max_count
            while repeat >= max_count:
                pixels = [value]
                info = (self.CMD_FILL, max_count, pixels)
                infos.append(info)
                repeat -= max_count

            # Is it still interesting to use FILL commands?
            if repeat >= threshold:
                pixels = [value]
                info = (self.CMD_FILL, repeat, pixels)
                infos.append(info)
            # No, use COPY commands
            elif repeat:
                pixels = []
                for _ in range(repeat):
                    pixels.append(value)

                # Can we merge next pixels into this COPY command?
                while index < len(pairs) and pairs[index][0] < threshold:
                    repeat2, value2 = pairs[index]
                    index += 1

                    while repeat2 > 0:
                        pixels.append(value2)
                        repeat2 -= 1

                        if len(pixels) == max_count:
                            info = (self.CMD_COPY, len(pixels), pixels)
                            infos.append(info)
                            pixels = []

                # Store remaining pixels
                if len(pixels) > 0:
                    info = (self.CMD_COPY, len(pixels), pixels)
                    infos.append(info)

        if self.verbose:
            sys.stdout.write(f"Nb values: {len(infos)}\n")
            for command, repeat, pixels in infos:
                if command == self.CMD_FILL:
                    value = pixels[0]
                    sys.stdout.write(f"FILL {repeat:3d} x 0x{value:02X}\n")
                else:
                    sys.stdout.write(f"COPY {repeat:3d} x {pixels}\n")

        return infos

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The provided packed values will contain:
        - Command (Fill or Copy)
        - Repeat count
        - Value: value(s) to be Filled/Copied
        """
        pairs = []
        for command, repeat, pixels in data:
            if command == self.CMD_FILL:
                value = pixels[0]
                pairs.append((repeat, value))
            else:
                # Store pixel by pixel => duplicates will be merged later
                for value in pixels:
                    pairs.append((1, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs

    # -------------------------------------------------------------------------
    def get_encoded_size(self, data):
        """
        Return the encoded size.
        """
        # Size needed to store the commands
        cmd_size = len(data) // 8
        if len(data) % 8:
            cmd_size += 1

        # Size needed to store the repeat count
        count_size = len(data) // 2
        if len(data) % 2:
            count_size += 1

        # Compute size needed by pixels
        total_pixels = 0
        bits_for_pixels = 0
        for command, repeat, pixels in data:
            total_pixels += repeat
            if command == self.CMD_FILL:
                bits_for_pixels += self.bpp
            else:
                bits_for_pixels += self.bpp * repeat
                assert repeat == len(pixels)

        # Size needed by pixels
        pixels_size = bits_for_pixels // 8
        if bits_for_pixels % 8:
            pixels_size += 1

        sys.stdout.write(f"Nb pixels: {total_pixels}\n")
        sys.stdout.write(f"sizes: cmd={cmd_size}, count={count_size}"\
                         f", data={pixels_size}\n")

        return cmd_size + count_size + pixels_size

# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustomA (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass) for 1 BPP only
    The generated bytes will contain Alternances counts ZZZZOOOO with
    - ZZZZ: number of consecutives 0, from 0 to 15
    - OOOO: number of consecutives 1, from 0 to 15
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        if self.bpp != 1:
            sys.stdout.write("The class RLECustomA is for 1 BPP data only!\n")
            sys.exit(1)

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=15):
        """
        Encode Alternance counts between pixels (nb of 0, then nb of 1, etc)
        The generated packed values will contain ZZZZOOOO ZZZZOOOO with
        - ZZZZ: number of consecutives 0, from 0 to 15
        - OOOO: number of consecutives 1, from 0 to 15
        """
        self.bpp = self.bpp

        # First step: store alternances, and split if count > 15
        next_pixel = 0
        alternances = []
        for repeat, value in pairs:
            # Store a count of 0 pixel if next pixel is not the one expected
            if value != next_pixel:
                alternances.append(0)
                next_pixel ^= 1

            while repeat > max_count:
                # Store 15 pixels of value next_pixel
                alternances.append(max_count)
                repeat -= max_count
                # Store 0 pixel of alternate value
                alternances.append(0)

            if repeat:
                alternances.append(repeat)
                next_pixel ^= 1

        if self.verbose:
            sys.stdout.write(f"Nb values: {len(alternances)}\n")
            next_pixel = 0
            for repeat in alternances:
                sys.stdout.write(f"{repeat:2d} x {next_pixel}\n")
                next_pixel ^= 1

        # Now read all those values and store them into quartets
        output = bytes()
        index = 0

        while index < len(alternances):
            zeros = alternances[index]
            index += 1
            if index < len(alternances):
                ones = alternances[index]
                index += 1
            else:
                ones = 0

            byte = (zeros << 4) | ones
            output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The provided packed values will contain ZZZZOOOO with
        - ZZZZ: number of consecutives 0, from 0 to 15
        - OOOO: number of consecutives 1, from 0 to 15
        """
        pairs = []
        for byte in data:
            ones = byte & 0x0F
            byte >>= 4
            zeros = byte & 0x0F
            if zeros:
                pairs.append((zeros, 0))
            if ones:
                pairs.append((ones, 1))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs

# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustomB (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass) for 1 BPP only
    The generated bytes will contain Alternances counts ZZZZZZZZ OOOOOOOO with
    - ZZZZZZZZ: number of consecutives 0, from 0 to 255
    - OOOOOOOO: number of consecutives 1, from 0 to 255
    TODO: check the same with 6 bits for repeat count
    ZZZZZZOO OOOOZZZZ ZZOOOOOO
    - ZZZZZZ: number of consecutives 0, from 0 to 63
    - OOOOOO: number of consecutives 1, from 0 to 63
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        if self.bpp != 1:
            sys.stdout.write("The class RLECustomB is for 1 BPP data only!\n")
            sys.exit(1)

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=255):
        """
        Encode Alternance counts between pixels (nb of 0, then nb of 1, etc)
        The generated packed values will contain ZZZZZZZZ OOOOOOOO with
        - ZZZZZZZZ: number of consecutives 0, from 0 to 255
        - OOOOOOOO: number of consecutives 1, from 0 to 255
        """
        self.bpp = self.bpp

        # First step: store alternances, and split if count > 255
        next_pixel = 0
        alternances = []
        for repeat, value in pairs:
            # Store a count of 0 pixel if next pixel is not the one expected
            if value != next_pixel:
                alternances.append(0)
                next_pixel ^= 1

            while repeat > max_count:
                # Store 255 pixels of value next_pixel
                alternances.append(max_count)
                repeat -= max_count
                # Store 0 pixel of alternate value
                alternances.append(0)

            if repeat:
                alternances.append(repeat)
                next_pixel ^= 1

        if self.verbose:
            sys.stdout.write(f"Nb values: {len(alternances)}\n")
            next_pixel = 0
            for repeat in alternances:
                sys.stdout.write(f"{repeat:2d} x {next_pixel}\n")
                next_pixel ^= 1

        # Now read all those values and store them into bytes
        output = bytes()
        index = 0

        while index < len(alternances):
            zeros = alternances[index]
            index += 1
            if index < len(alternances):
                ones = alternances[index]
                index += 1
            else:
                ones = 0

            output += bytes([zeros])
            output += bytes([ones])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The generated bytes will contain Alternances counts ZZZZZZZZ OOOOOOOO with
        - ZZZZZZZZ: number of consecutives 0, from 0 to 255
        - OOOOOOOO: number of consecutives 1, from 0 to 255
        """
        pairs = []
        for index, repeat in enumerate(data):
            if index & 1:
                pairs.append((repeat, 1))
            else:
                pairs.append((repeat, 0))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack patterns + command, repeat count & value separated
# -----------------------------------------------------------------------------
class RLECustomPC (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass) for 4bpp data
    The generated bytes will contain separated cmd + data information:

        CMD + DATA:

        1000 + VVVV => Simple Pattern Black to White: 0x0, VVVV, 0xF
        1001 + VVVV => Simple Pattern White to Black: 0xF, VVVV, 0x0
        1010 + VVVV + WWWW => Double Pattern Black to White: 0x0, VVVV, WWWW, 0xF
        1011 + VVVV + WWWW => Double Pattern White to Black: 0xF, VVVV, WWWW, 0x0
        111R + RRRR => FILL White (max=32)
        1100 + RRRR => FILL Black (max=16)
        1101 + VVVV => Fill Value x 1

        00RR + VVVV + WWWW + XXXX + YYYY + ZZZZ => COPY Quartets
        - RR is repeat count - 2 of quartets (max=2+3 => 5 quartets)
        - VVVV: value of 1st 4BPP pixel
        - WWWW: value of 2nd 4BPP pixel
        - XXXX: value of 3rd 4BPP pixel
        - YYYY: value of 4th 4BPP pixel
        - ZZZZ: value of 5th 4BPP pixel

        0100 + VVVV + RRRR => FILL Value x Repeat+3 (max=18)
        0101 + VVVV => FILL Value x 2
        0110 + IIII => Double Pattern Indexed Black to White: 0x0, Val1, val2, 0xF
        0111 + IIII => Double Pattern Indexed White to Black: 0xF, Val2, Val1, 0x0
    """
    patterns = {}
    sorted_patterns = {}

    # -------------------------------------------------------------------------
    def __init__(self, bpp=4, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        self.commands = []
        self.repeat = []
        self.values = []

        if self.bpp != 4:
            sys.stdout.write("The class RLECustomPC is for 4 BPP data only!\n")
            sys.exit(1)

    # -------------------------------------------------------------------------
    def add_double_pattern_black(self, value1, value2):
        """
        Add a double pattern, from black to white, if it doesn't exist yet
        black, value1, value2, white
        value1 is supposed to be smaller than value2, but that's not mandatory
        """
        pattern = (value1, value2)
        if pattern in RLECustomPC.patterns:
            count = RLECustomPC.patterns[pattern]
            RLECustomPC.patterns[pattern] = count + 1
        else:
            # that pattern was not found: add it!
            RLECustomPC.patterns[pattern] = 1

        return pattern

    # -------------------------------------------------------------------------
    def add_double_pattern_white(self, value1, value2):
        """
        Add a double pattern, from white to black, if it doesn't exist yet
        white, value1, value2, black
        value1 is supposed to be greater than value2, but that's not mandatory
        """
        pattern = (value2, value1)
        if pattern in RLECustomPC.patterns:
            count = RLECustomPC.patterns[pattern]
            RLECustomPC.patterns[pattern] = count + 1
        else:
            # that pattern was not found: add it!
            RLECustomPC.patterns[pattern] = 1

        return pattern

    # -------------------------------------------------------------------------
    def get_max_count(self, value, repeat):
        """
        Return the max_count allowed for provided value.
        (typically, 0xF is allowed a bigger max_count than for example 3)
        """
        if value == 15:
            return 32

        if value == 0:
            # Special case for color 0:
            # 1 to 16 repeat need 1 quartet
            # 1 or 2 repeat need 1 quartet
            # 3 to 18 repeat need 2 quartets
            # => 1 to 16 repeat : 16 (1 quartet)
            # => 17 repeat =18 (2 quartets), 18 repeat =18 (2 quartets)
            # => 19 repeat =16+3 (1+1 quartets), etc
            if repeat in (17, 18):
                return 18

            return 16

        # 1 or 2 need 1 quartet
        # 3 to 18 need 2 quartets
        return 18

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=32):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated packed values will contain:
        - Command (Fill or Copy or Pattern)
        - Repeat count
        - Value: value(s) to be Filled/Copied
        """
        #sys.stdout.write(f"For pass\n")
        # First, look for simple patterns: black, value, white or white, value, black
        for i in range (1, len(pairs)-1):
            # Check if we have a pattern 0x0, 1x value, 0xF
            repeat0, value0 = pairs[i-1]
            repeat, value = pairs[i]
            repeat1, value1 = pairs[i+1]
            if repeat != 1 or repeat0 < 1 or repeat1 < 1:
                continue
            # is previous value black and next white?
            if value0 == 0x0 and value1 == 0xF:
                # Cool, we found a simple black pattern!
                pairs[i] = (-1, value)  # -1 = CMD_SIMPLE_PATTERN_BLACK
                # Update previous (black) & next (white) pairs
                pairs[i-1] = (repeat0-1, value0)
                pairs[i+1] = (repeat1-1, value1)
                # Skip next index
                i += 1
                # if next value repeat count is 0, skip it
                if repeat1 == 1:
                    i += 1

            # is previous value white and next black?
            elif value0 == 0xF and value1 == 0x0:
                # Cool, we found a simple white pattern!
                pairs[i] = (-2, value)  # -2 = CMD_SIMPLE_PATTERN_WHITE
                # Update previous (white) & next (black) pairs
                pairs[i-1] = (repeat0-1, value0)
                pairs[i+1] = (repeat1-1, value1)
                # Skip next index
                i += 1
                # if next value repeat count is 0, skip it
                if repeat1 == 1:
                    i += 1

        # Second, look for double patterns: black, val1, val2, white or white, val1, val2, black
        for i in range (1, len(pairs)-2):
            # Check if we have a pattern 0x0, 1x value1, 1x value2, 0xF
            repeat0, value0 = pairs[i-1]
            repeat, value = pairs[i]
            repeat1, value1 = pairs[i+1]
            repeat2, value2 = pairs[i+2]
            if repeat != 1 or repeat1 != 1 or repeat0 < 1 or repeat2 < 1:
                continue
            # is previous value black and next white?
            if value0 == 0x0 and value2 == 0xF:
                # Cool, we found a double black pattern!
                pattern = self.add_double_pattern_black(value, value1)
                pairs[i] = (-3, pattern) # -3 = CMD_DOUBLE_PATTERN_BLACK
                pairs[i+1] = (0, 0)     # That one is not needed anymore
                # Update previous (black) & next (white) pairs
                pairs[i-1] = (repeat0-1, value0)
                pairs[i+2] = (repeat2-1, value2)
                # Skip next 2 index
                i += 2
                # if next value repeat count is 0, skip it
                if repeat2 == 1:
                    i += 1

            # is previous value white and next black?
            elif value0 == 0xF and value2 == 0x0:
                # Cool, we found a double white pattern!
                pattern = self.add_double_pattern_white(value, value1)
                pairs[i] = (-4, pattern) # -4 = CMD_DOUBLE_PATTERN_WHITE
                pairs[i+1] = (0, 0)     # That one is not needed anymore
                # Update previous (white) & next (black) pairs
                pairs[i-1] = (repeat0-1, value0)
                pairs[i+2] = (repeat2-1, value2)
                # Skip next 2 index
                i += 2
                # if next value repeat count is 0, skip it
                if repeat2 == 1:
                    i += 1
        infos = []
        index = 0
        threshold = 2

        #sys.stdout.write(f"\nWhile pass\n")
        while index < len(pairs):
            #sys.stdout.write(f"index={index}, pair={pairs[index]}\n")
            repeat, value = pairs[index]
            pixels = [value]
            index += 1

            # This one doesn't exist anymore, just skip this index
            if repeat == 0:
                continue

            # Do we have some pattern around?
            if repeat < 0:
                if repeat == -1:
                    info = (self.CMD_SIMPLE_PATTERN_BLACK, 1, pixels)
                    infos.append(info)
                    repeat = 0
                elif repeat == -2:
                    info = (self.CMD_SIMPLE_PATTERN_WHITE, 1, pixels)
                    infos.append(info)
                    repeat = 0
                elif repeat == -3:
                    info = (self.CMD_DOUBLE_PATTERN_BLACK, 1, pixels)
                    infos.append(info)
                    repeat = 0
                elif repeat == -4:
                    info = (self.CMD_DOUBLE_PATTERN_WHITE, 1, pixels)
                    infos.append(info)
                    repeat = 0
                else:
                    sys.stdout.write("repeat is < 0 but its neither -1, -2, -3 or -4!!\n")
                    sys.exit(1)

            # We can't store more than a repeat count of max_count
            while repeat >= self.get_max_count(value, repeat):
                info = (self.CMD_FILL, self.get_max_count(value, repeat), pixels)
                infos.append(info)
                repeat -= self.get_max_count(value, repeat)

            # Is it still interesting to use FILL commands?
            if repeat >= threshold:
                info = (self.CMD_FILL, repeat, pixels)
                infos.append(info)

            # No, use COPY commands
            elif repeat: # Here, repeat == 1
                pixels = []
                for _ in range(repeat):
                    pixels.append(value)

                # Store remaining pixels
                if len(pixels) > 0:
                    info = (self.CMD_COPY, len(pixels), pixels)
                    infos.append(info)

        # Merge contiguous COPY instructions
        i = 0
        while i < (len(infos)-1):
            command, repeat, pixels = infos[i]
            command1, repeat1, pixels1 = infos[i+1]
            # Be sure repeat count does not exceed 5
            if command == self.CMD_COPY and command1 == self.CMD_COPY and repeat < 5:
                assert repeat1 == 1
                for value in pixels1:
                    pixels.append(value)
                infos[i] = (self.CMD_COPY, len(pixels), pixels)
                infos.pop(i+1)
                continue
            i += 1

        # Replace remaining single COPY 1x value by FILL 1x value
        for i, info in enumerate(infos):
            command, repeat, pixels = info
            if command == self.CMD_COPY and repeat == 1:
                infos[i] = (self.CMD_FILL, 1, pixels)

        if self.verbose:
            sys.stdout.write(f"Nb values: {len(infos)}\n")
            for command, repeat, pixels in infos:
                if command == self.CMD_FILL:
                    value = pixels[0]
                    sys.stdout.write(f"FILL {repeat:3d} x 0x{value:X}\n")
                elif command == self.CMD_SIMPLE_PATTERN_BLACK:
                    value = pixels[0]
                    sys.stdout.write(f"SIMPLE_PATTERN_BLACK {repeat:3d} x (0x0, {value}, 0xF)\n")
                elif command == self.CMD_SIMPLE_PATTERN_WHITE:
                    value = pixels[0]
                    sys.stdout.write(f"SIMPLE_PATTERN_WHITE {repeat:3d} x (0xF, {value}, 0x0)\n")
                elif command == self.CMD_DOUBLE_PATTERN_BLACK:
                    pattern = pixels[0]
                    value1, value2 = pattern
                    sys.stdout.write(f"DOUBLE_PATTERN_BLACK {repeat:3d} x"
                                     f"(0x0, {value1}, {value2}, 0xF)\n")
                elif command == self.CMD_DOUBLE_PATTERN_WHITE:
                    pattern = pixels[0]
                    value2, value1 = pattern
                    sys.stdout.write(f"DOUBLE_PATTERN_WHITE {repeat:3d} x"
                                     f"(0xF, {value1}, {value2}, 0x0)\n")
                else:
                    sys.stdout.write(f"COPY {repeat:3d} x {pixels}\n")

        return infos

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The provided packed values will contain:
        - Command (Fill, Copy, pattern etc)
        - Repeat count
        - Value: value(s) to be Filled/Copied
        """
        pairs = []
        for command, repeat, pixels in data:
            if command == self.CMD_FILL:
                value = pixels[0]
                pairs.append((repeat, value))
            elif command == self.CMD_SIMPLE_PATTERN_BLACK:
                value = pixels[0]
                pairs.append((1, 0x0))
                pairs.append((1, value))
                pairs.append((1, 0xF))
            elif command == self.CMD_SIMPLE_PATTERN_WHITE:
                value = pixels[0]
                pairs.append((1, 0xF))
                pairs.append((1, value))
                pairs.append((1, 0x0))
            elif command == self.CMD_DOUBLE_PATTERN_BLACK:
                pattern = pixels[0]
                value1, value2 = pattern
                pairs.append((1, 0x0))
                pairs.append((1, value1))
                pairs.append((1, value2))
                pairs.append((1, 0xF))
            elif command == self.CMD_DOUBLE_PATTERN_WHITE:
                pattern = pixels[0]
                value2, value1 = pattern
                pairs.append((1, 0xF))
                pairs.append((1, value1))
                pairs.append((1, value2))
                pairs.append((1, 0x0))
            else:
                assert command == self.CMD_COPY
                # Store pixel by pixel => duplicates will be merged later
                for value in pixels:
                    pairs.append((1, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        # Display pairs
        if self.verbose and False:
            sys.stdout.write("After decode_pass2, pairs are:\n")
            sys.stdout.write(f"Nb values on pass1: {len(pairs)}\n")
            for repeat, value in pairs:
                sys.stdout.write(f"{repeat:3d} x 0x{value:02X}\n")

        return pairs

    # -------------------------------------------------------------------------
    @staticmethod
    def sort_patterns():
        """
        That method sort RLECustomPC.patterns and store the result in RLECustomPC.sorted_patterns
        """

        # To sort the dictionary using count value
        def get_count(item):
            _, count = item
            return count

        #RLECustomPC.sorted_patterns = dict(
        #    sorted(RLECustomPC.patterns.items(), key=lambda item: item[1], reverse=True))
        RLECustomPC.sorted_patterns = sorted(RLECustomPC.patterns.items(),
                                             key=get_count, reverse=True)

    # -------------------------------------------------------------------------
    @staticmethod
    def optimize():
        """
        That method sort RLECustomPC.patterns to be able to use indexed double patterns.
        For best results, it must be called only when we parsed all possible characters,
        to have the correct count values for RLECustomPC.patterns
        """
        # Sort the double patterns if its not already done
        if len(RLECustomPC.sorted_patterns) == 0:
            RLECustomPC.sort_patterns()

    # -------------------------------------------------------------------------
    def get_double_pattern_index(self, double_pattern):
        """
        Get the index of that double_pattern value, in RLECustomPC.sorted_patterns
        """
        # Return an invalid value if double patterns are not sorted yet
        if len(RLECustomPC.sorted_patterns) == 0:
            return -1

        i=0
        for entry in RLECustomPC.sorted_patterns:
            pattern, _ = entry
            if pattern == double_pattern:
                break
            i += 1

        return i

    # -------------------------------------------------------------------------
    def get_encoded_size(self, data):
        """
        Return the encoded size.
        """
        # Compute size needed
        total_pixels = 0
        cmd_needed   = 0
        data_needed  = 0
        for command, repeat, pixels in data:
            if command == self.CMD_FILL:
                value = pixels[0]
                if value == 0xF:
                    cmd_needed += 1
                    data_needed += 1
                elif value == 0:
                    cmd_needed += 1
                    if repeat <= 16:
                        data_needed += 1
                    else:
                        data_needed += 2
                elif repeat <= 2:
                    cmd_needed += 1
                    data_needed += 1
                else:
                    cmd_needed += 1
                    data_needed += 2
                total_pixels += repeat
            elif command == self.CMD_SIMPLE_PATTERN_BLACK:
                cmd_needed += 1
                data_needed += 1
                total_pixels += 1 + 1 + 1
            elif command == self.CMD_SIMPLE_PATTERN_WHITE:
                cmd_needed += 1
                data_needed += 1
                total_pixels += 1 + 1 + 1
            elif command == self.CMD_DOUBLE_PATTERN_BLACK:
                # if its a double pattern frequently used, we just need 1 byte
                pattern = pixels[0]
                index = self.get_double_pattern_index(pattern)
                if 0 <= index < 16:
                    data_needed += 1
                else:
                    data_needed += 2
                cmd_needed += 1
                total_pixels += 1 + 1 + 1 + 1
            elif command == self.CMD_DOUBLE_PATTERN_WHITE:
                # if its a double pattern frequently used, we just need 1 byte
                pattern = pixels[0]
                index = self.get_double_pattern_index(pattern)
                if 0 <= index < 16:
                    data_needed += 1
                else:
                    data_needed += 2
                cmd_needed += 1
                total_pixels += 1 + 1 + 1 + 1
            else: # CMD_COPY
                cmd_needed += 1
                data_needed += repeat
                total_pixels += repeat

        sys.stdout.write(f"Nb pixels: {total_pixels}\n")

        # Return the number of bytes needed (nb quartets/2)
        size = cmd_needed + data_needed
        size += 1
        size //= 2
        return size


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack patterns + command, repeat count & value separated
# -----------------------------------------------------------------------------
class RLECustom5 (RLECustomPC):
    """
    Class handling custom RLE encoding (2nd pass) for 4bpp data
    The generated bytes will contain separated cmd + data information:

        CMD + DATA:

        1000 + VVVV => Simple Pattern Black to White: 0x0, VVVV, 0xF
        1001 + VVVV => Simple Pattern White to Black: 0xF, VVVV, 0x0
        1010 + VVVV + WWWW => Double Pattern Black to White: 0x0, VVVV, WWWW, 0xF
        1011 + VVVV + WWWW => Double Pattern White to Black: 0xF, VVVV, WWWW, 0x0
        111R + RRRR => FILL White (max=32)
        1100 + RRRR => FILL Black (max=16)
        1101 + VVVV => Fill Value x 1

        00RR + VVVV + WWWW + XXXX + YYYY + ZZZZ => COPY Quartets
        - RR is repeat count - 2 of quartets (max=2+3 => 5 quartets)
        - VVVV: value of 1st 4BPP pixel
        - WWWW: value of 2nd 4BPP pixel
        - XXXX: value of 3rd 4BPP pixel
        - YYYY: value of 4th 4BPP pixel
        - ZZZZ: value of 5th 4BPP pixel

        0100 + VVVV + RRRR => FILL Value x Repeat+3 (max=18)
        0101 + VVVV => FILL Value x 2
        0110 + IIII => Double Pattern Indexed Black to White: 0x0, Val1, val2, 0xF
        0111 + IIII => Double Pattern Indexed White to Black: 0xF, Val2, Val1, 0x0
    """
    patterns = {}
    sorted_patterns = {}
    best_patterns = bytes()

    # -------------------------------------------------------------------------
    def __init__(self, bpp=4, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        self.commands = []
        self.repeat = []
        self.values = []

        if self.bpp != 4:
            sys.stdout.write("The class RLECustom5 is for 4 BPP data only!\n")
            sys.exit(1)

    # -------------------------------------------------------------------------
    def add_double_pattern_black(self, value1, value2):
        """
        Add a double pattern, from black to white, if it doesn't exist yet
        black, value1, value2, white
        value1 is supposed to be smaller than value2, but that's not mandatory
        """
        pattern = (value1, value2)
        if pattern in RLECustom5.patterns:
            count = RLECustom5.patterns[pattern]
            RLECustom5.patterns[pattern] = count + 1
        else:
            # that pattern was not found: add it!
            RLECustom5.patterns[pattern] = 1

        return pattern

    # -------------------------------------------------------------------------
    def add_double_pattern_white(self, value1, value2):
        """
        Add a double pattern, from white to black, if it doesn't exist yet
        white, value1, value2, black
        value1 is supposed to be greater than value2
        """
        pattern = (value2, value1)
        if pattern in RLECustom5.patterns:
            count = RLECustom5.patterns[pattern]
            RLECustom5.patterns[pattern] = count + 1
        else:
            # that pattern was not found: add it!
            RLECustom5.patterns[pattern] = 1

        return pattern

    # -------------------------------------------------------------------------
    @staticmethod
    def sort_patterns():
        """
        That method sort RLECustom5.patterns and store the result in RLECustom5.sorted_patterns
        """

        # To sort the dictionary using count value
        def get_count(item):
            _, count = item
            return count

        #RLECustom5.sorted_patterns = dict(
        #    sorted(RLECustom5.patterns.items(), key=lambda item: item[1], reverse=True))
        RLECustom5.sorted_patterns = sorted(RLECustom5.patterns.items(),
                                            key=get_count, reverse=True)

    # -------------------------------------------------------------------------
    @staticmethod
    def optimize():
        """
        That method sort RLECustom5.patterns to be able to use indexed double patterns.
        For best results, it must be called only when we parsed all possible characters,
        to have the correct count values for RLECustom5.patterns
        """
        # Sort the double patterns if its not already done
        if len(RLECustom5.sorted_patterns) == 0:
            RLECustom5.sort_patterns()

        # Return an array of bytes containing the best double patterns
        RLECustom5.best_patterns = bytes()
        i=0
        for entry in RLECustom5.sorted_patterns:
            pattern, _ = entry
            value1, value2 = pattern
            byte = value1 << 4
            byte |= value2
            RLECustom5.best_patterns += bytes([byte])

            i += 1
            if i == 16:
                break

        return RLECustom5.best_patterns

    # -------------------------------------------------------------------------
    def get_double_pattern_index(self, double_pattern):
        """
        Get the index of that double_pattern value, in RLECustom5.sorted_patterns
        """
        # Return an invalid value if double patterns are not sorted yet
        if len(RLECustom5.sorted_patterns) == 0:
            return -1

        i=0
        for entry in RLECustom5.sorted_patterns:
            pattern, _ = entry
            if pattern == double_pattern:
                break
            i += 1

        return i

    # -------------------------------------------------------------------------
    def add_quartet(self, quartet, output, byte, nb_bits):
        """
        Store provided quartet, by updating byte & nb_bits values.
        Add byte to output if we have 8 bits stored in byte.
        """
        self.bpp = self.bpp

        assert 0 <= quartet <= 15

        if nb_bits == 0:
            byte = quartet << 4
            nb_bits = 4
        else:
            byte |= quartet
            output += bytes([byte])
            byte = 0
            nb_bits = 0

        return output, byte, nb_bits

    # -------------------------------------------------------------------------
    def get_quartet(self, index, data, byte, nb_bits):
        """
        Get the next quartet.
        """
        self.bpp = self.bpp

        quartet = 0

        if nb_bits == 8:
            quartet = byte >> 4         # Get the msb
            nb_bits = 4

        elif nb_bits == 4:
            quartet = byte & 0x0F       # Get the lsb
            # Update byte with next 2 quartets
            if index < len(data):
                byte = data[index]
                index += 1
                nb_bits = 8
            else:
                nb_bits = 0

        return quartet, index, data, byte, nb_bits

    # -------------------------------------------------------------------------
    def encode_pass3(self, infos):
        """
        Encode tuples containing (command, repeat, pixels) into packed values.
        The generated bytes will contain:
        1000 + VVVV => Simple Pattern Black to White: 0x0, VVVV, 0xF
        1001 + VVVV => Simple Pattern White to Black: 0xF, VVVV, 0x0
        1010 + VVVV + WWWW => Double Pattern Black to White: 0x0, VVVV, WWWW, 0xF
        1011 + VVVV + WWWW => Double Pattern White to Black: 0xF, VVVV, WWWW, 0x0
        111R + RRRR => FILL White (max=32)
        1100 + RRRR => FILL Black (max=16)
        1101 + VVVV => Fill Value x 1

        00RR + VVVV + WWWW + XXXX + YYYY + ZZZZ => COPY Quartets
        - RR is repeat count - 2 of quartets (max=2+3 => 5 quartets)
        - VVVV: value of 1st 4BPP pixel
        - WWWW: value of 2nd 4BPP pixel
        - XXXX: value of 3rd 4BPP pixel
        - YYYY: value of 4th 4BPP pixel
        - ZZZZ: value of 5th 4BPP pixel

        0100 + VVVV + RRRR => FILL Value x Repeat+3 (max=18)
        0101 + VVVV => FILL Value x 2
        0110 + IIII => Double Pattern Indexed Black to White: 0x0, Val1, val2, 0xF
        0111 + IIII => Double Pattern Indexed White to Black: 0xF, Val2, Val1, 0x0
        """
        output = bytes()
        byte = 0
        nb_bits = 0
        # Parse infos values
        for command, repeat, pixels in infos:
            assert repeat > 0
            value = pixels[0]
            if command == self.CMD_FILL:
                if value == 0xF:        # CMD=111R + RRRR => FILL White (max=32)
                    repeat -= 1
                    cmd = 0x0E          # 1110
                    if repeat >= 16:
                        cmd = 0x0F      # 1111
                        repeat -= 16
                    output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(repeat, output, byte, nb_bits)

                elif value == 0:
                    if repeat <= 16:    # CMD=1100 + RRRR => FILL Black (max=16)
                        repeat -= 1
                        cmd = 0x0C      # 1100
                        output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                        output, byte, nb_bits = self.add_quartet(repeat, output, byte, nb_bits)
                    else:               # CMD=0100 + VVVV + RRRR => FILL Value x Repeat+3 (max=18)
                        repeat -= 3
                        cmd = 0x04      # 0100
                        output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                        # VVVV is 0x0000 here
                        output, byte, nb_bits = self.add_quartet(0x00, output, byte, nb_bits)
                        output, byte, nb_bits = self.add_quartet(repeat, output, byte, nb_bits)
                elif repeat <= 2:
                    if repeat == 1:     # CMD=1101 + VVVV => Fill Value x 1
                        cmd = 0x0D
                        output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                        output, byte, nb_bits = self.add_quartet(value, output, byte, nb_bits)
                    else:               # CMD=0101 + VVVV => FILL Value x 2
                        cmd = 0x05
                        output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                        output, byte, nb_bits = self.add_quartet(value, output, byte, nb_bits)
                else:                   # CMD=0100 + VVVV + RRRR => FILL Value x Repeat+3 (max=18)
                    repeat -= 3
                    cmd = 0x04      # 0100
                    output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(value, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(repeat, output, byte, nb_bits)

            # CMD=1000 + VVVV => Simple Pattern Black to White: 0x0, VVVV, 0xF
            elif command == self.CMD_SIMPLE_PATTERN_BLACK:
                cmd = 0x08      # 1000
                output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                output, byte, nb_bits = self.add_quartet(value, output, byte, nb_bits)
            # CMD=1001 + VVVV => Simple Pattern White to Black: 0xF, VVVV, 0x0
            elif command == self.CMD_SIMPLE_PATTERN_WHITE:
                cmd = 0x09      # 1001
                output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                output, byte, nb_bits = self.add_quartet(value, output, byte, nb_bits)
            # CMD=1010 + VVVV + WWWW => Double Pattern Black to White: 0x0, VVVV, WWWW, 0xF
            # or
            # CMD=0110 + IIII => Double Pattern Indexed Black to White: 0x0, Val1, val2, 0xF

            elif command == self.CMD_DOUBLE_PATTERN_BLACK:
                # For the moment, store value1 & value2
                pattern = pixels[0]
                index = self.get_double_pattern_index(pattern)
                if 0 <= index < 16:
                    cmd = 0x06      # 0110
                    output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(index, output, byte, nb_bits)
                else:
                    cmd = 0x0A      # 1010
                    value1, value2 = pattern
                    output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(value1, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(value2, output, byte, nb_bits)
            # CMD=1011 + VVVV + WWWW => Double Pattern White to Black: 0xF, VVVV, WWWW, 0x0
            # or
            # CMD=0111 + IIII => Double Pattern Indexed White to Black: 0xF, Val2, Val1, 0x0
            elif command == self.CMD_DOUBLE_PATTERN_WHITE:
                # For the moment, store value1 & value2
                pattern = pixels[0]
                index = self.get_double_pattern_index(pattern)
                if 0 <= index < 16:
                    cmd = 0x07      # 0111
                    output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(index, output, byte, nb_bits)
                else:
                    cmd = 0x0B      # 1011
                    value1, value2 = pattern
                    output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(value1, output, byte, nb_bits)
                    output, byte, nb_bits = self.add_quartet(value2, output, byte, nb_bits)
            # CMD=00RR + VVVV + WWWW + XXXX + YYYY + ZZZZ => COPY Quartets
            # - RR is repeat count - 2 of quartets (max=2+3 => 5 quartets)
            # - VVVV: value of 1st 4BPP pixel
            # - WWWW: value of 2nd 4BPP pixel
            # - XXXX: value of 3rd 4BPP pixel
            # - YYYY: value of 4th 4BPP pixel
            # - ZZZZ: value of 5th 4BPP pixel
            else: # CMD_COPY
                cmd = 0                 # 0000
                repeat -= 2
                cmd += repeat           # 00RR
                output, byte, nb_bits = self.add_quartet(cmd, output, byte, nb_bits)
                assert len(pixels) > 0 and len(pixels) <= 5
                for value in pixels:
                    output, byte, nb_bits = self.add_quartet(value, output, byte, nb_bits)

        # Add last quartet to bytes array, if there are remaining bits to add
        if nb_bits != 0:
            output, byte, nb_bits = self.add_quartet(0, output, byte, nb_bits)

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        1000 + VVVV => Simple Pattern Black to White: 0x0, VVVV, 0xF
        1001 + VVVV => Simple Pattern White to Black: 0xF, VVVV, 0x0
        1010 + VVVV + WWWW => Double Pattern Black to White: 0x0, VVVV, WWWW, 0xF
        1011 + VVVV + WWWW => Double Pattern White to Black: 0xF, VVVV, WWWW, 0x0
        111R + RRRR => FILL White (max=32)
        1100 + RRRR => FILL Black (max=16)
        1101 + VVVV => Fill Value x 1

        00RR + VVVV + WWWW + XXXX + YYYY + ZZZZ => COPY Quartets
        - RR is repeat count - 2 of quartets (max=2+3 => 5 quartets)
        - VVVV: value of 1st 4BPP pixel
        - WWWW: value of 2nd 4BPP pixel
        - XXXX: value of 3rd 4BPP pixel
        - YYYY: value of 4th 4BPP pixel
        - ZZZZ: value of 5th 4BPP pixel

        0100 + VVVV + RRRR => FILL Value x Repeat+3 (max=18)
        0101 + VVVV => FILL Value x 2
        0110 + IIII => Double Pattern Indexed Black to White: 0x0, Val1, val2, 0xF
        0111 + IIII => Double Pattern Indexed White to Black: 0xF, Val2, Val1, 0x0
        """
        pairs = []
        if not data:
            return pairs

        byte = data[0]
        index = 1
        nb_bits = 8

        while index < len(data) or nb_bits:
            # Next quartet contains the command
            cmd, index, data, byte, nb_bits = self.get_quartet(index, data, byte, nb_bits)

            # Was it the last quartet?
            if cmd == 0 and index >= len(data) and nb_bits == 0:
                break

            # Is it a COPY command?
            if cmd in (0x00, 0x01, 0x02, 0x03):
                # CMD=00RR + VVVV + WWWW + XXXX + YYYY + ZZZZ => COPY Quartets
                # - RR is repeat count - 2 of quartets (max=2+3 => 5 quartets)
                # - VVVV: value of 1st 4BPP pixel
                # - WWWW: value of 2nd 4BPP pixel
                # - XXXX: value of 3rd 4BPP pixel
                # - YYYY: value of 4th 4BPP pixel
                # - ZZZZ: value of 5th 4BPP pixel
                repeat = cmd & 3
                repeat += 2
                while repeat > 0:
                    value, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                    pairs.append((1, value))
                    repeat -= 1

            # Is it a FILL command?
            elif cmd in (0x04, 0x05, 0x0C, 0x0D, 0x0E, 0x0F):
                # CMD=0100 + VVVV + RRRR => FILL Value x Repeat+3 (max=18)
                if cmd == 0x04:
                    value, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                    repeat, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                    repeat += 3

                # CMD=0101 + VVVV => FILL Value x 2
                elif cmd == 0x05:
                    value, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                    repeat = 2

                # CMD=1100 + RRRR => FILL Black (max=16)
                elif cmd == 0x0C:
                    value = 0x00
                    repeat, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                    repeat += 1

                # CMD=1101 + VVVV => Fill Value x 1
                elif cmd == 0x0D:
                    value, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                    repeat = 1

                # CMD=111R + RRRR => FILL White (max=32)
                else:
                    value = 0x0F
                    repeat, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                    repeat += 1
                    if cmd == 0x0F:
                        repeat += 16

                pairs.append((repeat, value))

            # CMD=0110 + IIII => Double Pattern Indexed Black to White: 0x0, Val1, val2, 0xF
            elif cmd == 0x06:
                indice, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                pattern = RLECustom5.best_patterns[indice]
                value1 = (pattern >> 4) & 0x0F
                value2 = pattern & 0x0F
                pairs.append((1, 0x0))
                pairs.append((1, value1))
                pairs.append((1, value2))
                pairs.append((1, 0xF))

            # CMD=0111 + IIII => Double Pattern Indexed White to Black: 0xF, Val2, Val1, 0x0
            elif cmd == 0x07:
                indice, index, data, byte, nb_bits = self.get_quartet(
                        index, data, byte, nb_bits)
                pattern = RLECustom5.best_patterns[indice]
                value2 = (pattern >> 4) & 0x0F
                value1 = pattern & 0x0F
                pairs.append((1, 0xF))
                pairs.append((1, value1))
                pairs.append((1, value2))
                pairs.append((1, 0x0))

            # CMD=1000 + VVVV => Simple Pattern Black to White: 0x0, VVVV, 0xF
            elif cmd == 0x08:
                value, index, data, byte, nb_bits = self.get_quartet(
                    index, data, byte, nb_bits)
                pairs.append((1, 0x0))
                pairs.append((1, value))
                pairs.append((1, 0xF))

            # CMD=1001 + VVVV => Simple Pattern White to Black: 0xF, VVVV, 0x0
            elif cmd == 0x09:
                value, index, data, byte, nb_bits = self.get_quartet(
                    index, data, byte, nb_bits)
                pairs.append((1, 0xF))
                pairs.append((1, value))
                pairs.append((1, 0x0))

            # CMD=1010 + VVVV + WWWW => Double Pattern Black to White: 0x0, VVVV, WWWW, 0xF
            elif cmd == 0x0A:
                value1, index, data, byte, nb_bits = self.get_quartet(
                    index, data, byte, nb_bits)
                value2, index, data, byte, nb_bits = self.get_quartet(
                    index, data, byte, nb_bits)
                pairs.append((1, 0x0))
                pairs.append((1, value1))
                pairs.append((1, value2))
                pairs.append((1, 0xF))

            # CMD=1011 + VVVV + WWWW => Double Pattern White to Black: 0xF, WWWW, VVVV, 0x0
            elif cmd == 0x0B:
                value2, index, data, byte, nb_bits = self.get_quartet(
                    index, data, byte, nb_bits)
                value1, index, data, byte, nb_bits = self.get_quartet(
                    index, data, byte, nb_bits)
                pairs.append((1, 0xF))
                pairs.append((1, value1))
                pairs.append((1, value2))
                pairs.append((1, 0x0))
            else:
                sys.stdout.write(f"cmd={cmd} which is an impossible case!\n")
                sys.exit(112)

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs

    # -------------------------------------------------------------------------
    def get_encoded_size(self, data):
        """
        Return the encoded size.
        (to be overwritten if something more complex than len() need to be done)
        """
        self.bpp = self.bpp

        return len(data)

    # -------------------------------------------------------------------------
    def encode(self, data):
        """
        Input: array of packed pixels
        - convert to an array of values
        - convert to tuples of (repeat, value)
        - encode using custom RLE
        Output: array of compressed bytes
        """
        values = self.bpp_to_values(data)
        pairs = self.encode_pass1(values)

        # Second pass
        infos = self.encode_pass2(pairs)

        # Third & last pass
        self.encoded = self.encode_pass3(infos)

        # Decode to check everything is fine
        decompressed = self.decode(self.encoded)
        if decompressed != data:
            # This will never happen in prod environment, only when testing
            sys.stdout.write("Encoding/Decoding is WRONG!!!\n")
            sys.exit(111)

        return self.encoded

    # -------------------------------------------------------------------------
    def decode(self, data):
        """
        Input: array of compressed bytes
        - decode to pairs using custom RLE
        - convert the tuples of (repeat, value) to values
        - convert to an array of packed pixels
        Output: array of packed pixels
        """
        pairs = self.decode_pass2(data)
        values = self.decode_pass1(pairs)

        decoded = self.values_to_bpp(values)

        return decoded


# -----------------------------------------------------------------------------
# Entry point for easy RLE encoding/decoding
# -----------------------------------------------------------------------------
class RLECustom:
    """
    Class handling custom RLE encoding/decoding
    """
    # -------------------------------------------------------------------------
    @classmethod
    def encode(cls, packed_pixels, bpp, verbose=False):
        """
        Try different encoding algorithms to compress 4 or 1BPP pixels.
        Input:
        - array of packed pixels
        - number of bpp (1 or 4)
        Output: Tuple containing:
        - method used to encode data
        - bytes array containing encoded data
        """
        # Default values: no encoding
        method = 0
        compressed = packed_pixels
        # Try different methods depending on BPP
        if bpp == 4:
            # For now, just compress 4 BPP bitmap with RLECustom5
            rle = RLECustom5(bpp, verbose)
            compressed = rle.encode(packed_pixels)
            method = 1

        elif bpp == 1:
            # For now, just compress 1 BPP bitmap with RLECustomA
            rle = RLECustomA(bpp, verbose)
            compressed = rle.encode(packed_pixels)
            method = 1

        return method, compressed

    # -------------------------------------------------------------------------
    @classmethod
    def decode(cls, method, encoded_data, bpp, verbose=False):
        """
        Decompress previously encoded data using the provided method.
        Input: array of compressed bytes
        Output: array of packed pixels
        """
        # Is data really encoded?
        decoded = encoded_data
        if method != 1:
            pass
        elif bpp == 4:
            # Just one method for the moment
            rle = RLECustom5(bpp, verbose)
            decoded = rle.decode(encoded_data)
        elif bpp == 1:
            # Just one method for the moment
            rle = RLECustomA(bpp, verbose)
            decoded = rle.decode(encoded_data)

        return decoded

    # -------------------------------------------------------------------------
    @classmethod
    def optimize(cls):
        """
        That method sort RLECustom5.patterns to be able to use indexed double patterns.
        For best results, it must be called only when we parsed all possible characters,
        to have the correct count values for RLECustom5.patterns
        """
        # Should return bytes() if RLECustom5 class was not used at all (bpp=1 or other method)
        return RLECustom5.optimize()

# -------------------------------------------------------------------------
def get_bitmap_characters(file):
    """
    Parse the provided JSON file(s) and return bitmap + characters info
    """
    if not os.path.exists(file):
        sys.stderr.write(f"Can't open JSON file {file}\n")
        sys.exit(2)

    # Read the JSON file into json_data
    with open(file, "r", encoding="utf-8", errors='ignore') as json_file:
        json_data = json.load(json_file, strict=False)

    if json_data and 'bitmap' in json_data[0]:
        bitmap = base64.b64decode(json_data[0]['bitmap'])
    else:
        sys.stderr.write(f"Didn't found field 'bitmap' in JSON file {file}!\n")
        sys.exit(3)

    if 'nbgl_font_character' in json_data[0]:
        characters = json_data[0]['nbgl_font_character']
    elif 'bagl_font_unicode_character' in json_data[0]:
        characters = json_data[0]['bagl_font_unicode_character']
    elif 'nbgl_font_unicode_character' in json_data[0]:
        characters = json_data[0]['nbgl_font_unicode_character']
    else:
        characters = None
        sys.stderr.write(f"Didn't found field 'XX_font_character' in JSON file {file}!\n")
        sys.exit(3)

    return bitmap, characters

# -------------------------------------------------------------------------
def get_data(bitmap, characters, character):
    """
    Return the data corresponding the character provided
    """
    data = bytearray()
    for i in range (0, len(characters)):
        if characters[i]['char'] == character:
            offset = characters[i]['bitmap_offset']
            if i == len(characters)-1:
                size = len(bitmap) - offset
            else:
                size = characters[i+1]['bitmap_offset'] - offset
            for j in range (0, size):
                data.append(bitmap[offset+j])
            return data
    return None

# -------------------------------------------------------------------------
def compress_data(args, encoded_data, just_compress=False):
    """
    Try to compress the provided data.
    (We first need to uncompress it, to get original data)
    """
    # Uncompress the data, to get the uncompressed size & data
    if encoded_data is None and just_compress is False:
        sys.stdout.write("encoded_data is None!\n")
        return 0, 0, 0

    if len(encoded_data) == 0 and just_compress is False:
        sys.stdout.write("No data to encode.\n")
        return 0, 0, 0

    rle = RLECustom3(args.bpp, args.verbose)
    data = rle.decode(encoded_data)

    if just_compress is False:
        ratio = 100 - (100 * len(encoded_data)) / len(data)
        sys.stdout.write(f"Input size: {len(data)} bytes ({len(data)*2} pixels)\n")
        sys.stdout.write(f"Encoded size: {len(encoded_data)} bytes "
                     f"(ratio:{ratio:.2f}%)\n")
        verbose = args.verbose
    else:
        verbose = False

    # Now, try the new algorithm
    rle_new = RLECustom5(args.bpp, verbose)
    compressed_data = rle_new.encode(data)
    compressed_size = len(compressed_data)
    if just_compress is False:
        compressed_size = rle_new.get_encoded_size(compressed_data)

        ratio = 100 - (100 * compressed_size) / len(data)

        sys.stdout.write(f"New encoded size: {compressed_size} bytes "
                         f"(ratio:{ratio:.2f}%)\n")

    # No need to check if decoding is fine, already done when encoding
    return len(data), len(encoded_data), compressed_size


# -----------------------------------------------------------------------------
# Program entry point:
# -----------------------------------------------------------------------------
if __name__ == "__main__":

    # -------------------------------------------------------------------------
    def main(args):
        """
        Main method.
        """
        # Regular (ASCII) JSON files are at public_sdk/lib_nbgl/include/fonts/
        # nbgl_font_inter_regular_28.json
        # nbgl_font_inter_regular_28_1bpp.json
        # nbgl_font_inter_medium_36.json
        # nbgl_font_inter_medium_36_1bpp.json
        # nbgl_font_inter_semibold_28.json
        # nbgl_font_inter_semibold_28_1bpp.json
        # bagl_font_Inter_Medium_36px.json
        # bagl_font_Inter_Medium_36px_1bpp.json
        # bagl_font_Inter_Regular_28px_unicode.json
        # bagl_font_Inter_Regular_28px_1bpp_unicode.json
        # bagl_font_Inter_SemiBold_28px_unicode.json
        # bagl_font_Inter_SemiBold_28px_1bpp_unicode.json

        # Chinese, RU & FR JSON files are at /home/dmorais/gitea/dmo-backup
        # bagl_font_NotoSerifSC_Regular_28px_unicode.json
        # bagl_font_NotoSerifSC_SemiBold_28px_unicode.json
        # bagl_font_NotoSerifSC_Medium_36px_unicode.json
        # bagl_font_NotoSerifSC_Regular_28px_1bpp_unicode.json
        # bagl_font_NotoSerifSC_SemiBold_28px_1bpp_unicode.json
        # bagl_font_NotoSerifSC_Medium_36px_1bpp_unicode.json
        #
        # nbgl_font_Inter_Medium_36px_fr.json
        # nbgl_font_Inter_Regular_28px_fr.json
        # nbgl_font_Inter_SemiBold_28px_fr.json
        #
        # nbgl_font_Inter_Medium_36px_ru.json
        # nbgl_font_Inter_Regular_28px_ru.json
        # nbgl_font_Inter_SemiBold_28px_ru.json

        # compressed ascii 0x0040 inter_medium_36.inc

        # nbgl_font_inter_medium_36.json
        # They contain characters from 0x20 to 0x7E

        bitmap, characters = get_bitmap_characters(args.json_filename)

        total_raw_size = 0
        total_old_size = 0
        total_new_size = 0
        nb_characters  = 0

        # è is 0xE8 (232)
        if args.char is not None:
            encoded_data = get_data(bitmap, characters, args.char)
            sys.stdout.write(f"Encoding character 0x{args.char:X} ({args.char:d})[{args.char:c}]\n")
            raw_size, old_size, new_size = compress_data(args, encoded_data)
            total_raw_size += raw_size
            total_old_size += old_size
            total_new_size += new_size
            nb_characters += 1
        else:
            # First pass, just compress to find frequently used double patterns
            for character in characters:
                char = character['char']
                encoded_data = get_data(bitmap, characters, char)
                compress_data(args, encoded_data, just_compress=True)
            # Sort dictionary of patterns
            RLECustom5.optimize()

            # Now, do the full job
            for character in characters:
                char = character['char']
                encoded_data = get_data(bitmap, characters, char)
                sys.stdout.write(f"\nEncoding character 0x{char:X} ({char:d})[{char:c}]\n")
                raw_size, old_size, new_size = compress_data(args, encoded_data)
                total_raw_size += raw_size
                total_old_size += old_size
                total_new_size += new_size
                nb_characters += 1

        sys.stdout.write(f"Number of encoded characters: {nb_characters}\n")

        # Take in account the various dictionaries
        if len(RLECustom5.sorted_patterns) != 0:
            sys.stdout.write(f"Number of double patterns: {len(RLECustom5.sorted_patterns)}\n")
            i=0
            patterns_count = 0
            for entry in RLECustom5.sorted_patterns:
                _, count = entry
                patterns_count += count
                i += 1
                if i >= 16:
                    break

            sys.stdout.write(f"Size of first 16 double patterns: {patterns_count}\n")
            # Store the number of double quartets used
            total_new_size += i

        if total_raw_size:
            old_ratio = 100 - (100 * total_old_size) / total_raw_size
            new_ratio = 100 - (100 * total_new_size) / total_raw_size
            sys.stdout.write(f"Original size = {total_raw_size} bytes\n")
            sys.stdout.write(f"Old size={total_old_size}(ratio:{old_ratio:.2f}%), "
                             f"new_size={total_new_size}(ratio:{new_ratio:.2f}%)\n")

        return 0

    # -------------------------------------------------------------------------
    # Parse arguments:
    parser = argparse.ArgumentParser(
        description="Test custom RLE methods (Build #220223.1003)")

    parser.add_argument(
        "-b", "--bpp",
        dest="bpp", type=int,
        default=4,
        help="Number of bits per pixel ('%(default)s' by default)")

    parser.add_argument(
        "-c", "--char",
        dest="char", type=int,
        default=None,
        help="Character to encode ('%(default)s' by default)")

    parser.add_argument(
        "-j", "--json",
        dest="json_filename", type=str,
        default="lib_nbgl/include/fonts/nbgl_font_inter_medium_36.json",
        help="Full path of JSON filename containing bitmap/char info ('%(default)s' by default)")

    parser.add_argument(
        "-v", "--verbose",
        action = 'store_true',
        help="Add verbosity to output ('%(default)s' by default)")

    # Call main function:
    EXIT_CODE = main(parser.parse_args())

    sys.exit(EXIT_CODE)
