
//////////////////////
// sim65_testcase.h //
//////////////////////

#ifndef SIM65_TESTCASE_H
#define SIM65_TESTCASE_H

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

enum sim65_cpu_mode_type {
    SIM65_CPU_6502,
    SIM65_CPU_65C02,
    SIM65_CPU_6502X
};

int execute_testcase(struct sim65_testcase_specification_type * testcase, const char * filename, unsigned testcase_index, enum sim65_cpu_mode_type cpu_mode);

#endif
