
//////////////////////
// sim65_testcase.h //
//////////////////////

#ifndef SIM65_TESTCASE_H
#define SIM65_TESTCASE_H

#include <inttypes.h>

struct machine_state_type
{
    uint16_t pc;
    uint8_t s;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t p;
    uint8_t ram[0x10000];
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
