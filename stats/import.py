#!/usr/bin/env python3

import argparse

from Import import Import


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "-h", "--help",
        action="help",
        help="Shows this short help text and exits the program."
    )
    parser.add_argument(
        "--fileConfig", "-c",
        action="store",
        help="Full path to configuration file",
        required=True
    )
    args = parser.parse_args()
    fileConfig = args.fileConfig
    
    try:
        imp = Import(pathFileConfig=fileConfig)
    except Exception as e:
        print(e)
        exit(1)
    
    exit(0)
