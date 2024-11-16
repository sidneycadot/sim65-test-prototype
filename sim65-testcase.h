
//////////////////////
// sim65_testcase.h //
//////////////////////

#ifndef SIM65_TESTCASE_H
#define SIM65_TESTCASE_H

//#include <stdio.h>
//#include <stdlib.h>
//#include <assert.h>
//#include <string.h>

//#include "6502.h"
//#include "memory.h"

struct machine_state_type
{
    unsigned pc;
    unsigned s;
    unsigned a;
    unsigned x;
    unsigned y;
    unsigned p;
    unsigned char ram[0x10000];
};

struct sim65_testcase_specification_type
{
    char * name;
    struct machine_state_type initial_state;
    struct machine_state_type final_state;
    unsigned cycles;
};

int execute_testcase(struct sim65_testcase_specification_type * testcase, const char * filename, unsigned testcase_index);

#endif
