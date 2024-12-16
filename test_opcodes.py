#! /usr/bin/env python3

"""Test legal/documented opcodes, which are the only ones that sim65 aims to support."""

import argparse
import os
import subprocess
import multiprocessing
import itertools

from make_testresult_dashboard import make_testresult_dashboard

sim65_supported_opcodes = {
    "6502": """
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
            """,
    "6502X": """
            00 01 .. 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
            10 11 .. 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f
            20 21 .. 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f
            30 31 .. 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f
            40 41 .. 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f
            50 51 .. 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f
            60 61 .. 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f
            70 71 .. 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f
            80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f
            90 91 .. 93 94 95 96 97 98 99 9a 9b 9c 9d 9e 9f
            a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af
            b0 b1 .. b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf
            c0 c1 c2 c3 c4 c5 c6 c7 c8 c9 ca cb cc cd ce cf
            d0 d1 .. d3 d4 d5 d6 d7 d8 d9 da db dc dd de df
            e0 e1 e2 e3 e4 e5 e6 e7 e8 e9 ea eb ec ed ee ef
            f0 f1 .. f3 f4 f5 f6 f7 f8 f9 fa fb fc fd fe ff
            """,
    "65C02": """
            00 01 02 03 04 05 06 .. 08 09 0a 0b 0c 0d 0e ..
            10 11 12 13 14 15 16 .. 18 19 1a 1b 1c 1d 1e ..
            20 21 22 23 24 25 26 .. 28 29 2a 2b 2c 2d 2e ..
            30 31 32 33 34 35 36 .. 38 39 3a 3b 3c 3d 3e ..
            40 41 42 43 44 45 46 .. 48 49 4a 4b 4c 4d 4e ..
            50 51 52 53 54 55 56 .. 58 59 5a 5b 5c 5d 5e ..
            60 61 62 63 64 65 66 .. 68 69 6a 6b 6c 6d 6e ..
            70 71 72 73 74 75 76 .. 78 79 7a 7b 7c 7d 7e ..
            80 81 82 83 84 85 86 .. 88 89 8a 8b 8c 8d 8e ..
            90 91 92 93 94 95 96 .. 98 99 9a 9b 9c 9d 9e ..
            a0 a1 a2 a3 a4 a5 a6 .. a8 a9 aa ab ac ad ae ..
            b0 b1 b2 b3 b4 b5 b6 .. b8 b9 ba bb bc bd be ..
            c0 c1 c2 c3 c4 c5 c6 .. c8 c9 ca .. cc cd ce ..
            d0 d1 d2 d3 d4 d5 d6 .. d8 d9 da .. dc dd de ..
            e0 e1 e2 e3 e4 e5 e6 .. e8 e9 ea eb ec ed ee ..
            f0 f1 f2 f3 f4 f5 f6 .. f8 f9 fa fb fc fd fe ..
            """
}

def test_sim65_supported_opcodes(sim65_cpu_variant_and_testcase_directory, sim65_version: str) -> None:

    (sim65_cpu_variant, testcase_directory) = sim65_cpu_variant_and_testcase_directory

    print("Starting test:", sim65_version, sim65_cpu_variant, testcase_directory)

    sim65_supported_opcodes_for_cpu = sim65_supported_opcodes[sim65_cpu_variant]

    testfiles = [os.path.join(testcase_directory, f"{opcode}.json") for opcode in sim65_supported_opcodes_for_cpu.split() if opcode != ".."]

    if sim65_version == "original":
        executable = "./sim65-original-test"
    elif sim65_version == "fixed":
        executable = "./sim65-fixed-test"
    else:
        raise ValueError()

    extra_args = []
    #extra_args = ["--disable-cycle-count-test"]

    result = subprocess.run([executable, f"--cpu-mode={sim65_cpu_variant}"] + extra_args + testfiles, capture_output=True, encoding='ascii')

    assert result.returncode == 0
    assert len(result.stderr) == 0

    filename = sim65_version + "_" + sim65_cpu_variant + "_" + testcase_directory + ".test-out"
    filename = filename.replace("/", "-")

    print("Writing result file:", filename)
    with open(filename, "w") as fo:
        fo.write(result.stdout)

def main() -> None:
    """Test 6502 CPU variants against 65x02 instruction behavior descriptions."""

    sim65_cpu_variants_and_testcase_directories = (
        ("6502", "65x02/6502/v1"),
        ("6502X", "65x02/6502/v1"),
        #("65C02", "65x02/rockwell65c02/v1"),
        #("65C02", "65x02/synertek65c02/v1"),
        ("65C02", "65x02/wdc65c02/v1"),
    )
    sim65_versions = ("original", "fixed")

    pool = multiprocessing.Pool()
    pool.starmap(test_sim65_supported_opcodes, itertools.product(sim65_cpu_variants_and_testcase_directories, sim65_versions))

    make_testresult_dashboard(sim65_cpu_variants_and_testcase_directories, "test_summary.html")

if __name__ == "__main__":
    main()