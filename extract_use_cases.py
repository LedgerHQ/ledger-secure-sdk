#!/usr/bin/env python3

from argparse import ArgumentParser, Namespace
import toml


COMMANDS = ["names", "flags"]
DEFAULT_USE_CASES = ["debug", "release"]


# ===============================================================================
#          Parse command line options
# ===============================================================================
def arg_parse() -> Namespace:
    """Parse the commandline options"""

    parser = ArgumentParser("Handle Apps use cases")
    parser.add_argument('-m', '--manifest', required=True, type=str, help="Manifest file to process")
    parser.add_argument('-c', '--command', required=True, type=str, choices=COMMANDS, help="Command to execute")
    parser.add_argument('-k', '--key', type=str, help="Use-Case to retrieve")
    return parser.parse_args()


# ===============================================================================
#          Processing the uses-cases and flags
# ===============================================================================
def print_result(args: Namespace) -> None:
    """Processing function

    Args:
        args: The command line arguments.
    """

    # Load manifest file
    # ------------------
    try:
        manifest = toml.load(f"{args.manifest}")
    except FileNotFoundError:
        print("")
        return

    # Retrieve use-cases from manifest or empty string
    # ------------------------------------------------
    use_cases = None
    names = ""
    try:
        use_cases = manifest.get("use_cases", None)
        names = " ".join(use_cases.keys())
    except (KeyError, AttributeError):
        pass

    # Check command and return expected values or empty string
    # --------------------------------------------------------
    if args.command == "names":
        names_set = set(names.split())  # Convert existing names to a set
        names_set.update(DEFAULT_USE_CASES)  # Add default cases
        print(" ".join(sorted(names_set)))
        return

    # If key not found, return empty string
    flags = ""
    if args.command == "flags":
        if args.key not in names:
            # If key is not found, check default use-cases
            if args.key == "debug":
                flags = "DEBUG=1"
        else:
            # Retrieve flags from manifest
            flags = use_cases.get(args.key, "")
        print(flags)


# ===============================================================================
#          MAIN
# ===============================================================================
def main() -> None:
    """Main function"""

    # Arguments parsing
    # -----------------
    args = arg_parse()

    # Arguments checking
    # ------------------
    if args.command == "values" and args.key is None:
        print("Key is required for 'values' command.")
        return

    # Processing
    # ----------
    print_result(args)


if __name__ == "__main__":
    main()
