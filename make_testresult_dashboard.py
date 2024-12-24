#! /usr/bin/env python3

"""Parse the output of a 'test_legal_opcodes.py' run and generate an HTML table summarizing the results."""

import itertools
import contextlib
import sys
import re
from typing import NamedTuple

class Result(NamedTuple):
    error_count: int
    test_count: int

def parse_file(filename: str):

    pattern = re.compile(".*/([0-9a-f]{2})\\.json.*INFO - Test file summary: ([0-9]+) of ([0-9]+) .*", re.ASCII | re.DOTALL)
    results = {}
    summary_error_count = 0
    summary_test_count = 0

    with open(filename, "r") as fi:
        for line in fi:
            match = pattern.fullmatch(line)
            if match is None:
                continue
            opcode = match.group(1)
            error_count = int(match.group(2))
            test_count = int(match.group(3))
            results[opcode] = Result(error_count, test_count)
            summary_error_count += error_count
            summary_test_count += test_count

    summary = Result(summary_error_count, summary_test_count)

    return (results, summary)

def render_results_as_table(sim65_cpu_variant, testcase_directory, results, summary, fo):
    with contextlib.redirect_stdout(fo):
        print(f"    <h1>{sim65_cpu_variant} / {testcase_directory}</h1>")
        print("    <table border=\"1\" style=\"text-align:center\">")
        for row in range(16):
            print("      <tr>", end='')
            for col in range(16):
                opcode = f"{row*16+col:02x}"
                result = results.get(opcode)
                if result is None:
                    print("<td style=\"color:lightgray\">illegal</td>", end='')
                else:
                    bad_perc = result.error_count / result.test_count * 100.0
                    if bad_perc==0.0:
                        color="lightgreen"
                    elif bad_perc < 1.0:
                        color="yellow"
                    elif bad_perc < 100.0:
                        color="orange"
                    else:
                        color="orangered"
                    print(f"<td style=\"background-color:{color}\">0x{opcode}:&nbsp;({bad_perc:.2f}%)</td>", end='')
            print("</tr>")
        print("    </table>")
        print(f"    <p>In total, {summary.error_count} out of {summary.test_count} tests failed ({summary.error_count/summary.test_count*100.0:.3f}%).</h1>")


def make_testresult_dashboard(sim65_cpu_variants_and_testcase_directories, html_filename: str):

    with open(html_filename, "w") as fo:
        with contextlib.redirect_stdout(fo):
            print("<html>")
            print("  <head>")
            print("    <title>sim65 results for the 65x02 test suite</title>")
            print("  <head>")
            print("  <body>")

        for (sim65_cpu_variant, testcase_directory) in sim65_cpu_variants_and_testcase_directories:
            filename = sim65_cpu_variant + "_" + testcase_directory + ".test-out"
            filename = filename.replace("/", "-")
            print(f"Parsing {filename} ...")
            (results, summary) = parse_file(filename)

            render_results_as_table(sim65_cpu_variant, testcase_directory, results, summary, fo)

        with contextlib.redirect_stdout(fo):
            print("  </body>")
            print("</html>")

    print(f"Wrote HTML file: {html_filename}")
