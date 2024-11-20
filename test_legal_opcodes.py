#! /usr/bin/env python3

"""Test legal/documented opcodes, which are the only ones that sim65 aims to support."""

import argparse
import os
import subprocess


def test_legal_6502_opcodes(version: str, testcase_directory: str) -> None:
    """Test the 151 vanilla 6502 opcodes that sim65 supports."""

    legal_6502_opcode_matrix = """
    00 01 .. .. .. 05 06 .. 08 09 0a .. .. 0d 0e ..
    10 11 .. .. .. 15 16 .. 18 19 .. .. .. 1d 1e ..
    20 21 .. .. 24 25 26 .. 28 29 2a .. 2c 2d 2e ..
    30 31 .. .. .. 35 36 .. 38 39 .. .. .. 3d 3e ..
    40 41 .. .. .. 45 46 .. 48 49 4a .. 4c 4d 4e ..
    50 51 .. .. .. 55 56 .. 58 59 .. .. .. 5d 5e ..
    60 61 .. .. .. 65 66 .. 68 69 6a .. 6c 6d 6e ..
    70 71 .. .. .. 75 76 .. 78 79 .. .. .. 7d 7e ..
    .. 81 .. .. 84 85 86 .. 88 .. 8a .. 8c 8d 8e ..
    90 91 .. .. 94 95 96 .. 98 99 9a .. .. 9d .. ..
    a0 a1 a2 .. a4 a5 a6 .. a8 a9 aa .. ac ad ae ..
    b0 b1 .. .. b4 b5 b6 .. b8 b9 ba .. bc bd be ..
    c0 c1 .. .. c4 c5 c6 .. c8 c9 ca .. cc cd ce ..
    d0 d1 .. .. .. d5 d6 .. d8 d9 .. .. .. dd de ..
    e0 e1 .. .. e4 e5 e6 .. e8 e9 ea .. ec ed ee ..
    f0 f1 .. .. .. f5 f6 .. f8 f9 .. .. .. fd fe ..
    """

    testfiles = [os.path.join(testcase_directory, f"{opcode}.json") for opcode in legal_6502_opcode_matrix.split() if opcode != ".."]

    if version == "original":
        executable = "./sim65-original-test"
    elif version == "fixed":
        executable = "./sim65-fixed-test"
    else:
        raise ValueError()

    subprocess.run([executable, "--cpu-mode=6502"] + testfiles)


def main() -> None:
    """Test vanilla 6502 opcodes."""

    parser = argparse.ArgumentParser()
    parser.add_argument("version", choices=("original", "fixed"))
    parser.add_argument("-t", "--testcase-directory", default="65x02/6502/v1")
    args = parser.parse_args()

    test_legal_6502_opcodes(args.version, args.testcase_directory)


if __name__ == "__main__":
    main()
