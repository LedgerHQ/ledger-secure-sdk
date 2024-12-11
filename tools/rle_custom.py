#!/usr/bin/env python3
# coding: utf-8
# -----------------------------------------------------------------------------
"""
This module contain tools to test different custom RLE coding/decoding.
"""
# -----------------------------------------------------------------------------
import argparse
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
    @staticmethod
    def remove_duplicates(pairs):
        """
        Check if there are some duplicated pairs (same values) and merge them.
        """
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
    @staticmethod
    def bpp4_to_values(data):
        """
        Expand each bytes of data into 2 quartets.
        Return an array of values (from 0x00 to 0x0F)
        """
        output = []
        for byte in data:
            lsb_bpp4 = byte & 0x0F
            byte >>= 4
            output.append(byte & 0x0F)
            output.append(lsb_bpp4)

        return output

    # -------------------------------------------------------------------------
    @staticmethod
    def values_to_bpp4(data):
        """
        Takes values (assumed from 0x00 to 0x0F) in data and returns an array
        of bytes containing quartets with values concatenated.
        """
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
    @staticmethod
    def bpp1_to_values(data):
        """
        Expand each bytes of data into 8 bits, each stored in a byte
        Return an array of values (containing bytes values 0 or 1)
        """
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
    @staticmethod
    def values_to_bpp1(data):
        """
        Takes values (bytes containing 0 or 1) in data and returns an array
        of bytes containing bits concatenated.
        (first pixel is bit 10000000 of first byte)
        """
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
        if self.bpp in (1, 4):
            pass

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

        if self.verbose:
            sys.stdout.write(f"Nb values on pass1: {len(output)}\n")
            for repeat, value in output:
                sys.stdout.write(f"{repeat:3d} x 0x{value:02X}\n")

        return output

    # -------------------------------------------------------------------------
    @staticmethod
    def decode_pass1(data):
        """
        Decode array of tuples containing (repeat, val).
        Return an array of values.
        """
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
        if self.bpp in (1, 4):
            pass

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
        if self.bpp in (1, 4):
            pass

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
        if self.bpp in (1, 4):
            pass

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
        if self.bpp in (1, 4):
            pass
        # First, generate data in 10RRRRRR/0RRRVVVV format
        single_output = RLECustom2.encode_pass2(pairs, max_count)

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
# Custom RLE encoding: pack repeat count & value into 1 byte +
# - white handling
# - copy range of bytes
# -----------------------------------------------------------------------------
class RLECustom3bis (RLECustom2):
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
        if self.bpp in (1, 4):
            pass

        # First, generate data in 10RRRRRR/0RRRVVVV format
        single_output = RLECustom2.encode_pass2(pairs, max_count)

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
        if self.bpp in (1, 4):
            pass

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
# Entry point for easy RLE encoding/decoding
# -----------------------------------------------------------------------------
class RLECustom:
    """
    Class handling custom RLE encoding
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
            # For now, just compress 4 BPP bitmap with RLECustom3
            rle = RLECustom3(bpp, verbose)
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
            rle = RLECustom3(bpp, verbose)
            decoded = rle.decode(encoded_data)
        elif bpp == 1:
            # Just one method for the moment
            rle = RLECustomA(bpp, verbose)
            decoded = rle.decode(encoded_data)

        return decoded

# -----------------------------------------------------------------------------
# Custom RLE encoding: pack patterns, repeat count & value
# -----------------------------------------------------------------------------
class RLECustomP (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass) for 4bpp data
    The generated bytes will contain triplets with:
    - Command (Fill or Copy or Pattern)
    - Repeat count
    - Value: value(s) to be Filled/Copied
    """

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
            sys.stdout.write("The class RLECustomP is for 4 BPP data only!\n")
            sys.exit(1)

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
            #sys.stdout.write(f"i={i}, pair-1={pairs[i-1]}, , pair={pairs[i]}, pair+1={pairs[i+1]}\n")
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
                else:
                    sys.stdout.write("repeat is < 0 but its neither -1 or -2!!\n")
                    sys.exit(1)

            # We can't store more than a repeat count of max_count
            while repeat >= max_count:
                info = (self.CMD_FILL, max_count, pixels)
                infos.append(info)
                repeat -= max_count

            # Is it still interesting to use FILL commands?
            if repeat >= threshold:
                info = (self.CMD_FILL, repeat, pixels)
                infos.append(info)

            # No, use COPY commands
            elif repeat:
                pixels = []
                for _ in range(repeat):
                    pixels.append(value)

                # Store remaining pixels
                if len(pixels) > 0:
                    info = (self.CMD_COPY, len(pixels), pixels)
                    infos.append(info)

        # Replace the COPY 1x 0x0 or COPY 1x 0xF by FILL 1x 0x0 or FILL 1x 0xF
        for i in range (0, len(infos)):
            command, repeat, pixels = infos[i]
            value = pixels[0]
            if command == self.CMD_COPY and repeat == 1 and (value == 0x0 or value == 0xF):
                infos[i] = (self.CMD_FILL, 1, pixels)

        # Merge contiguous COPY instructions
        i = 0
        while i < (len(infos)-1):
            command, repeat, pixels = infos[i]
            command1, repeat1, pixels1 = infos[i+1]
            if command == self.CMD_COPY and command1 == self.CMD_COPY:
                for value in pixels1:
                    pixels.append(value)
                infos[i] = (self.CMD_COPY, len(pixels), pixels)
                infos.pop(i+1)
                continue
            i += 1

        # Replace remaining single COPY 1x value by FILL 1x value
        for i in range (0, len(infos)):
            command, repeat, pixels = infos[i]
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
            else:
                # Store pixel by pixel => duplicates will be merged later
                for value in pixels:
                    pairs.append((1, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        # Display pairs
        sys.stdout.write("After decode_pass2, pairs are:\n")
        if self.verbose:
            sys.stdout.write(f"Nb values on pass1: {len(pairs)}\n")
            for repeat, value in pairs:
                sys.stdout.write(f"{repeat:3d} x 0x{value:02X}\n")


        return pairs

    # -------------------------------------------------------------------------
    def get_encoded_size(self, data):
        """
        Return the encoded size.
        """
        # Compute size needed
        total_pixels = 0
        quartets_needed = 0
        for command, repeat, pixels in data:
            if command == self.CMD_FILL:
                quartets_needed += 1 + 1
                total_pixels += repeat
            elif command == self.CMD_SIMPLE_PATTERN_BLACK:
                quartets_needed += 1 + 1
                total_pixels += 1 + 1 + 1
            elif command == self.CMD_SIMPLE_PATTERN_WHITE:
                quartets_needed += 1 + 1
                total_pixels += 1 + 1 + 1
            else: # CMD_COPY
                quartets_needed += 1 + repeat
                total_pixels += repeat

        bytes_needed = quartets_needed // 2
        if quartets_needed & 1:
            bytes_needed += 1

        sys.stdout.write(f"Nb pixels: {total_pixels}\n")

        return bytes_needed

# -----------------------------------------------------------------------------
# Program entry point:
# -----------------------------------------------------------------------------
if __name__ == "__main__":

    # -------------------------------------------------------------------------
    def main(args):
        """
        Main method.
        111RRRRR
        110RRRRR
        0RRRVVVV

        * 111RRRRR
          - RRRRR is repeat count - 1 of White (0xF) quartets (max=31+1)
        * 110RRRRR
          - RRRRR is repeat count - 1 of Black (0x0) quartets (max=31+1)
        * 0RRRVVVV
          - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
          - VVVV: value of the 4BPP pixel
        * 1000VVVV
          - Simple pattern, from black to white: 0x0, V, 0xF
        * 1001VVVV
          - Simple pattern, from white to black: 0xF, V, 0x0
        * 101XXXXX

        Coder un pattern qui va de 0, X, X, X, F et à l'inverse
                                de F, X, X, X, 0

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
        # compressed ascii 0x0040 inter_medium_36.inc
        encoded_data = bytes([
  0xCE, 0x2E, 0xDB, 0x8D, 0x72, 0x60, 0x82, 0x7C,
  0xD4, 0x0C, 0x05, 0x70, 0x40, 0x02, 0x0C, 0xD1,
  0x07, 0x70, 0x70, 0x00, 0x0A, 0xCE, 0x0E, 0x02,
  0x30, 0x92, 0x9C, 0xE0, 0xC1, 0x1E, 0x8C, 0xA7,
  0x30, 0x0E, 0xCD, 0x02, 0x20, 0x02, 0x0D, 0xCB,
  0x02, 0x10, 0x09, 0xCC, 0x05, 0x20, 0x07, 0xCD,
  0x0A, 0x10, 0x05, 0xCB, 0x0A, 0x20, 0x09, 0xCE,
  0x0C, 0x10, 0x05, 0xC4, 0x0E, 0xC5, 0x02, 0x10,
  0x05, 0xCE, 0x0E, 0x02, 0x10, 0x05, 0xC2, 0x07,
  0x12, 0xC4, 0x0A, 0x20, 0x0E, 0xC1, 0x09, 0x72,
  0x32, 0x30, 0x09, 0xC2, 0x02, 0x10, 0x0D, 0xC3,
  0x05, 0x10, 0x07, 0xC2, 0x09, 0x70, 0x60, 0x02,
  0x0E, 0xC2, 0x05, 0x10, 0x09, 0xC3, 0x20, 0x0D,
  0xC2, 0x09, 0x70, 0x50, 0x02, 0x0D, 0xC3, 0x09,
  0x10, 0x05, 0xC2, 0x0D, 0x10, 0x02, 0xC3, 0x0E,
  0x07, 0x20, 0x85, 0x9A, 0x1C, 0x09, 0x07, 0x20,
  0x09, 0xC4, 0x0D, 0x10, 0x02, 0xC2, 0x0A, 0x10,
  0x05, 0xC3, 0x0E, 0x20, 0x0A, 0xC6, 0x0E, 0x05,
  0x10, 0x09, 0xC3, 0x0E, 0x20, 0xC2, 0x09, 0x10,
  0x07, 0xC3, 0x07, 0x10, 0x05, 0xC8, 0x0E, 0x20,
  0x0E, 0xC3, 0x20, 0xC2, 0x09, 0x10, 0x09, 0xC3,
  0x02, 0x10, 0x0A, 0xC9, 0x05, 0x10, 0x09, 0xC3,
  0x02, 0x10, 0x0E, 0xC1, 0x09, 0x10, 0x07, 0xC3,
  0x02, 0x10, 0x0C, 0xC9, 0x07, 0x10, 0x07, 0xC3,
  0x20, 0x0E, 0xC1, 0x09, 0x10, 0x07, 0xC3, 0x02,
  0x10, 0x0A, 0xC9, 0x07, 0x10, 0x07, 0xC3, 0x20,
  0xC2, 0x0A, 0x10, 0x05, 0xC3, 0x05, 0x10, 0x07,
  0xC9, 0x02, 0x10, 0x09, 0xC2, 0x0E, 0x20, 0xC2,
  0x0D, 0x10, 0x02, 0xC3, 0x0A, 0x20, 0x0D, 0xC7,
  0x09, 0x20, 0x0C, 0xC2, 0x0A, 0x10, 0x02, 0xC3,
  0x20, 0x0D, 0xC3, 0x02, 0x20, 0x09, 0xC4, 0x0E,
  0x07, 0x20, 0x02, 0xC3, 0x07, 0x10, 0x05, 0xC3,
  0x05, 0x10, 0x09, 0xC3, 0x0C, 0x40, 0x25, 0x02,
  0x40, 0x0D, 0xC3, 0x02, 0x10, 0x09, 0xC3, 0x09,
  0x20, 0xC4, 0x0A, 0x70, 0x30, 0x0C, 0xC3, 0x09,
  0x20, 0x0E, 0xC4, 0x02, 0x10, 0x07, 0xC4, 0x0E,
  0x05, 0x70, 0x07, 0x0E, 0xC4, 0x02, 0x10, 0x05,
  0xC5, 0x09, 0x20, 0x0A, 0xC5, 0x8E, 0xA9, 0x17,
  0x09, 0x0C, 0xC6, 0x05, 0x20, 0x0D, 0xC6, 0x02,
  0x20, 0x0A, 0xD1, 0x07, 0x20, 0x05, 0xC7, 0x0D,
  0x30, 0x07, 0xCE, 0x0E, 0x05, 0x20, 0x02, 0xC9,
  0x0A, 0x30, 0x82, 0x9E, 0xC9, 0x0E, 0x07, 0x40,
  0x0D, 0xCA, 0x0A, 0x50, 0x82, 0x79, 0x2A, 0x19,
  0x05, 0x02, 0x40, 0x02, 0x0D, 0xCC, 0x0E, 0x02,
  0x70, 0x70, 0x10, 0x05, 0xD0, 0x0C, 0x05, 0x70,
  0x50, 0x05, 0x0D, 0xD3, 0x9D, 0x95, 0x20, 0x50,
  0x92, 0x59, 0xE0, 0xDA, 0x2E, 0xC0,
        ])

        # Uncompress the data, to get the uncompressed size & data
        rle = RLECustom3(args.bpp, args.verbose)
        data = rle.decode(encoded_data)

        ratio = 100 - (100 * len(encoded_data)) / len(data)
        sys.stdout.write(f"Input size: {len(data)} bytes ({len(data)*2} pixels)\n")
        sys.stdout.write(f"Encoded size: {len(encoded_data)} bytes "
                         f"(ratio:{ratio:.2f}%)\n")

        # Now, try the new algorithm
        rle_new = RLECustomP(args.bpp, args.verbose)
        compressed_data = rle_new.encode(data)
        compressed_size = rle_new.get_encoded_size(compressed_data)

        ratio = 100 - (100 * compressed_size) / len(data)

        sys.stdout.write(f"New encoded size: {compressed_size} bytes "
                         f"(ratio:{ratio:.2f}%)\n")

        # No need to check if decoding is fine, already done when encoding

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
        "-v", "--verbose",
        action = 'store_true',
        help="Add verbosity to output ('%(default)s' by default)")

    # Call main function:
    EXIT_CODE = main(parser.parse_args())

    sys.exit(EXIT_CODE)
