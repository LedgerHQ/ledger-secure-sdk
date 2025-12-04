#!/usr/bin/env python3
# coding: utf-8
# -----------------------------------------------------------------------------
"""
This module gives details on elf binary.
"""
# -----------------------------------------------------------------------------
from argparse import ArgumentParser
import os
import sys
import logging

from ledgered.binary import LedgerBinaryApp


logger = logging.getLogger(__name__)


# ===============================================================================
#          Parameters
# ===============================================================================
def elf_parser(input: str) -> None:
    binary = LedgerBinaryApp(input)

    print("---")
    logger.info("app_name:     '%s'", binary.sections.app_name)
    logger.info("app_version:  '%s'", binary.sections.app_version)
    logger.info("sdk_graphics: '%s'", binary.sections.sdk_graphics)

    print("---")
    logger.info("sdk_name:     '%s'", binary.sections.sdk_name)
    logger.info("sdk_version:  '%s'", binary.sections.sdk_version)
    logger.debug("api_level:   '%s'", binary.sections.api_level)
    logger.debug("sdk_hash:    '%s'", binary.sections.sdk_hash)

    print("---")
    logger.info("target:       '%s'", binary.sections.target)
    logger.info("target_id:    '%s'", binary.sections.target_id)
    logger.info("target_name:  '%s'", binary.sections.target_name)

    print("---")
    logger.info("app_flags:    '%s'", binary.sections.app_flags)


# ===============================================================================
#          Parameters
# ===============================================================================
def init_parser() -> ArgumentParser:
    parser = ArgumentParser(description="Parse ledger metadata from an elf file.")

    parser.add_argument("-i", "--input", required=True, type=str, help="Input elf file to process.")
    parser.add_argument("-v", "--verbose", action = 'store_true',
        help="Add verbosity to output ('%(default)s' by default)")
    return parser


# ===============================================================================
#          Main function
# ===============================================================================
def main() -> None:
    """
    Main method.
    """

    parser = init_parser()
    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(format="[%(levelname)s] %(message)s", level=logging.DEBUG)
    else:
        logging.basicConfig(format="[%(levelname)s] %(message)s", level=logging.INFO)

    if not os.access(args.input, os.R_OK):
        logger.error(f"Cannot read file {args.input}")
        sys.exit(1)

    elf_parser(args.input)


# ===============================================================================
#          Entry point
# ===============================================================================
if __name__ == "__main__":
    main()
