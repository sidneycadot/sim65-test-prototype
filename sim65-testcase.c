
//////////////////////
// sim65_testcase.c //
//////////////////////

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "6502.h"
#include "memory.h"

#include "sim65-testcase.h"

static bool sim65_reported_error;
static bool sim65_reported_warning;

/////////////////////////////////////////////////////////////////// start of re-implementation of functions that are called from 6502.c.

void ParaVirtHooks(CPURegs * Regs)
{
    // Our implementation of 'ParaVirtHooks' is a no-op.
    (void)Regs;
}

void Error(const char * Format, ...)
{
    (void)Format;
    sim65_reported_error = true;
}

void Warning(const char * Format, ...)
{
    (void)Format;
    sim65_reported_warning = true;
}

/////////////////////////////////////////////////////////////////// end of re-implementation of functions that are called from 6502.c.

static uint8_t fix_p_register_value(uint8_t p)
{
    // Test cases from the 65x02 test-set sometimes have their P register values set to different values (See: https://github.com/SingleStepTests/65x02/issues/8).
    // We set both bits 4 and 5 here which is what sim65 does.
    p |= 0x30;
    return p;
}

int execute_testcase(struct sim65_testcase_specification_type * testcase, const char * filename, unsigned testcase_index, enum sim65_cpu_mode_type cpu_mode, unsigned test_flags)
{
    switch (cpu_mode)
    {
        case SIM65_CPU_6502:
        {
            CPU = CPU_6502;
            break;
        }
        case SIM65_CPU_65C02:
        {
            CPU = CPU_65C02;
            break;
        }
        case SIM65_CPU_6502X:
        {
            CPU = CPU_6502X;
            break;
        }
        default:
        {
            // Bad CPU mode. This should never happen.
            assert(false);
            // In case assertions are disabled, at least proceed with a well-defined CPU type.
            CPU = CPU_6502;
        }
    }

    // Set the RESET vector, with a preceding JMP instruction.

    MemWriteByte(0xfffb, 0x4c);
    MemWriteWord(0xfffc, 0xfffb);

    // Reset the CPU. This will set the PC to the value found in (0xfffc, 0xfffd), which is 0xfffb.
    // There, a "jmp $fffb" instruction will be found.

    sim65_reported_error = false;
    sim65_reported_warning = false;

    // Initialize the sim65 state.

    Reset();

    Regs.AC = testcase->initial_state.a;
    Regs.XR = testcase->initial_state.x;
    Regs.YR = testcase->initial_state.y;
    Regs.SR = fix_p_register_value(testcase->initial_state.p);
    Regs.SP = testcase->initial_state.s;
    Regs.PC = testcase->initial_state.pc;

    // Initialize memory according to the initial (pre-instruction) state specified in the testcase.
    memcpy(Mem, testcase->initial_state.ram, 0x10000);

    // Run a single instruction.
    unsigned sim65_cyclecount = ExecuteInsn();

    // Verify state of CPU and memory and cycle count.

    unsigned errors_seen = 0;
    unsigned notices_seen = 0;

    if (sim65_reported_error)
    {
        printf("[%s:%u (\"%s\")] NOTICE - sim65 reported an illegal instruction at address 0x%04x and tried to halt execution.\n", filename, testcase_index, testcase->name, Regs.PC);
        ++notices_seen;
    }


    if (sim65_reported_warning)
    {
        printf("[%s:%u (\"%s\")] NOTICE - sim65 reported it encountered the JMP-indirect 6502 bug at address 0x%04x.\n", filename, testcase_index, testcase->name, Regs.PC);
        ++notices_seen;
    }

    if (Regs.AC != testcase->final_state.a)
    {
        printf("[%s:%u (\"%s\")] ERROR - A register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.a, Regs.AC);
        ++errors_seen;
    }

    if (Regs.XR != testcase->final_state.x)
    {
        printf("[%s:%u (\"%s\")] ERROR - X register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.a, Regs.XR);
        ++errors_seen;
    }

    if (Regs.YR != testcase->final_state.y)
    {
        printf("[%s:%u (\"%s\")] ERROR - Y register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.a, Regs.YR);
        ++errors_seen;
    }

    if (Regs.SR != fix_p_register_value(testcase->final_state.p))
    {
        printf("[%s:%u (\"%s\")] ERROR - P register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.p, Regs.SR);
        ++errors_seen;
    }

    if (Regs.SP != testcase->final_state.s)
    {
        printf("[%s:%u (\"%s\")] ERROR - S register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.s, Regs.SP);
        ++errors_seen;
    }

    if (Regs.PC != testcase->final_state.pc)
    {
        printf("[%s:%u (\"%s\")] ERROR - PC register check failed (expected: 0x%04x, sim65: 0x%04x).\n", filename, testcase_index, testcase->name, testcase->final_state.s, Regs.PC);
        ++errors_seen;
    }

    if ((test_flags & F_TEST_CYCLECOUNT) && sim65_cyclecount != testcase->cycles)
    {
        printf("[%s:%u (\"%s\")] ERROR - cycle count check failed (expected: %u, sim65: %u).\n", filename, testcase_index, testcase->name, testcase->cycles, sim65_cyclecount);
        ++errors_seen;
    }

    if ((test_flags & F_TEST_MEMORY) && memcmp(Mem, testcase->final_state.ram, 0x10000) != 0)
    {
        unsigned address;
        for (address = 0; testcase->final_state.ram[address] == Mem[address]; ++address)
        {
            // Find first address with a difference.
        }
        printf("[%s:%u (\"%s\")] ERROR - memory check failed: (address 0x%04x: expected 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name,
            address, testcase->final_state.ram[address], Mem[address]);
        ++errors_seen;
    }

    printf("[%s:%u (\"%s\")] INFO - Test summary: %u %s, %u %s.\n", filename, testcase_index, testcase->name,
            errors_seen, (errors_seen != 1) ? "errors" : "error",
            notices_seen, (notices_seen != 1) ? "notices" : "notice");

    return errors_seen ? -1 : 0;
}
