
sim65-test: a program to test the cc65's 6502 simulator (sim65)
===============================================================

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
- execute the single testcase instruction using sim65's "ExecuteInsn" instruction;
- verify the CPU registers, memory, and instruction cycle count against the values specified in the testcase.


How it works
------------

The CPU emulation in sim65 is built centered around the 'ExecuteInsn' function defined in 6502.c. For the purpose of
our tests, this is the function we want to verify.

This function is intended to be run from sim65 only, and as such it has some dependencies on externally available
functions:

- The 'ParaVirtHooks' function, normally defined in 'paravirt.c', is called at the end of "jump" instructions to
  provide a mechanism to interact with the outside world from within the emulated 6502 machine. This allows sim65
  programs that use the sim65-specific runtime library to access files from the machine on which it runs, for
  example.

- The 'Error' and 'Warning' functions, normally defined in 'errors.c', are called in specific situations. The
  'Error' function is called whenever an attempt is made to execute an illegal instruction; while the 'Warning'
  function is called when the JMP-indirect instruction is executed with the address LSB and MSB precisely split
  over two consecutive memory pages, which (on a "real" 6502) triggers a hardware bug where the MSB is actually
  read from the same page as the LSB.

  The standard implementation of the 'Error' function goes beyond reporting; it actively ends the sim65 process.

- Several 'Mem' functions are provided in 'memory.c' that are used to access 64 KB of system memory.

When we call 'ExecuteInsn' for testing, we provide alternative implementations of the functions above that
have different behavior compared to their counterparts when run from 'sim65':

- The 'ParaVirtHooks' function is redefined to merely copy the address of the struct holding the CPU state, which
  is defined in a static variable in 6502.c, into a variable that is accessible by the sim65-tester; otherwise,
  it does nothing.

  This is used as a way to gain access to the (more or less private) CPU state from within the tester, which is
  needed both to set up the CPU at the start of each test, and to verify the CPU state at the end of each test.

- The 'Error' and 'Warning' functions are redefined. The fact that they are called is reported by sim65-test;
  and sim65-test's the 'Error' function does not terminate execution of the process.

- The 'Mem' functions as implemented by sim65-test verify the address at which they are called, unlike their
  sim65 counterparts.

To enable the 'sim65-test' versions of these functions, minimalistic replacement header files are provided
('paravirt.h', error.h, memory.h) that only define prototypes for the functions we want. The sim65-test
specific versions for the functions are implemented in 'sim65-testcase.c'.

What this all means is that it allows the 'sim65-test' program to work with the original, unaltered versions
of '6502.c' and '6502.h' as found in cc65/src/sim65.


Status and future development
-----------------------------

For now, this is a prototype. If this is deemed useful enough, I will open an issue on the cc65 Github issue
tracker to see if we can somehow integrate this into the cc65 project.

But it's also fine to keep this as a seperate project as far as I am concerned.
