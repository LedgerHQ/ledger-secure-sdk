"""
Helper to extract APP_LOAD_PARAMS parameters values.
It takes as a first parameter the parameter name to be search and output the
corresponding values from the rest of the script parameters.

Exits with a non-zero status if the searched parameter is given without a
value, so that a malformed restriction (e.g. a trailing --curve or --path)
cannot silently be dropped and widen the generated install parameters.
"""

import sys

if __name__ == '__main__':

    assert len(sys.argv) > 2
    searching = sys.argv[1]

    res = []
    args = sys.argv[2:]

    for i, arg in enumerate(args):
        if arg != searching:
            continue
        if i + 1 >= len(args) or args[i + 1].startswith('--'):
            sys.exit(f"extract_param.py: missing value for {searching} in APP_LOAD_PARAMS")
        res.append(repr(args[i + 1]))
    print(" ".join(res))
