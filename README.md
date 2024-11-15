# sim65-test-prototype

This is a prototype for a test program to test the cc65 "sim65" 6502 simulator against the 65x02 test-suite.

Relevant repositories:

* https://github.com/cc65/cc65
* https://github.com/SingleStepTests/65x02
* https://github.com/DaveGamble/cJSON

The 65x02 repository contains a huge test-set of the behavior of all 6502 instructions (10,000 tests per instruction).

These are encoded in a simple JSON format.

Our program (sim65-test) will read test JSON files (using the cJSON library), iterate over all test cases, and for
each test case:

- initialize the CPU and memory according to the test case;
- execute a single instruction using sim65's "ExecuteInsn" instruction;
- verify the CPU registers, memory, and instruction cycle count against the values specified in the testcase.

Status and future development
-----------------------------

For now, this is a prototype, cobbled together from the relevant cc65 sources (with several ugly patches to make
it work). If this is deemed useful enough, I will open an issue on the cc65 github issue tracker to see if we can
somehow integrate this into the cc65 project.

But it's also fine to keep this as a seperate project as far as I am concerned.
