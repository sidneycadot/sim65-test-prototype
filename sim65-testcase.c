
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

// The CPURegs structure is a static variable defined in 6502.c.
// We obtain a pointer to it by (ab)using the 'ParaVirtHooks' function.
static CPURegs * cpu_state_ptr;

// Define memory array.
// The unsigned char base type is also used in sim65.
static unsigned char mem[0x10000]; 

static bool sim65_mem_write_byte_address_violation;
static bool sim65_mem_read_byte_address_violation;
static bool sim65_reported_error;
static bool sim65_reported_warning;

/////////////////////////////////////////////////////////////////// start of re-implementation of functions that are called from 6502.c.

void MemWriteByte(unsigned Addr, unsigned char Val)
{
    if (Addr > 0xffff)
    {
        sim65_mem_write_byte_address_violation = true;
    }
    else
    {
        mem[Addr] = Val;
    }
}

void MemWriteWord(unsigned Addr, unsigned Val)
{
    // Same implementation as sim65.
    MemWriteByte (Addr, Val & 0xFF);
    MemWriteByte (Addr + 1, Val >> 8);
}

unsigned char MemReadByte(unsigned Addr)
{
    if (Addr > 0xffff)
    {
        sim65_mem_read_byte_address_violation = true;
        return 0;
    }
    else
    {
        return mem[Addr];
    }
}

unsigned MemReadWord(unsigned Addr)
{
    // Same implementation as sim65.
    unsigned W = MemReadByte (Addr++);
    return (W | (MemReadByte (Addr) << 8));
}

unsigned MemReadZPWord(unsigned char Addr)
{
    // Same implementation as sim65.
    unsigned W = MemReadByte (Addr++);
    return (W | (MemReadByte (Addr) << 8));
}

void ParaVirtHooks(CPURegs * Regs)
{
    // Copy the pointer to the CPU state to our local 'cpu_state_ptr', which we can subsequently use to access the simulated CPU's registers.
    cpu_state_ptr = Regs;
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

int execute_testcase(struct sim65_testcase_specification_type * testcase, const char * filename, unsigned testcase_index, enum sim65_cpu_mode_type cpu_mode)
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

    Reset();

    // Executing a single instruction, which will be read from 0xfffb.
    // Since it is a jump instruction, this will trigger a 'ParaVirtHooks' invocation,
    // which will give us the location of the CPURegs struct used by 6502.c.

    cpu_state_ptr = NULL;

    sim65_mem_write_byte_address_violation = false;
    sim65_mem_read_byte_address_violation = false;
    sim65_reported_error = false;
    sim65_reported_warning = false;

    ExecuteInsn(); // Execute the single JMP instruction.

    // Verify that the JMP instruction didn't do anything naughty.

    assert(sim65_mem_write_byte_address_violation == false);
    assert(sim65_mem_read_byte_address_violation == false);
    assert(sim65_reported_error == false);
    assert(sim65_reported_warning == false);

    // The JMP instruction handler in 6502.c called our 'ParaVirtHooks' function (defined above),
    // which set the cpu_state_ptr to point to the 'Regs' static variable in 6502.c.
    // Verify that this did, indeed, happen.

    assert(cpu_state_ptr != NULL);

    // Initialize the sim65 state from the "initial" state.
    // Note: the "ZR" field seems vestigial. Initialize it to zero anyway.

    cpu_state_ptr->AC = testcase->initial_state.a;
    cpu_state_ptr->XR = testcase->initial_state.x;
    cpu_state_ptr->YR = testcase->initial_state.y;
    cpu_state_ptr->ZR = 0;
    cpu_state_ptr->SR = testcase->initial_state.p;
    cpu_state_ptr->SP = testcase->initial_state.s;
    cpu_state_ptr->PC = testcase->initial_state.pc;

    // Initialize memory according to the initial (pre-instruction) state specified in the testcase.
    memcpy(mem, testcase->initial_state.ram, 0x10000);

    // Run a single instruction.
    unsigned sim65_cyclecount = ExecuteInsn();

    // Verify state of CPU and memory and cycle count.

    unsigned errors_seen = 0;
    unsigned notices_seen = 0;

    if (sim65_reported_error)
    {
        printf("[%s:%u (\"%s\")] NOTICE - sim65 reported an illegal instruction at address 0x%04x and tried to halt execution.\n", filename, testcase_index, testcase->name, cpu_state_ptr->PC);
        ++notices_seen;
    }


    if (sim65_reported_warning)
    {
        printf("[%s:%u (\"%s\")] NOTICE - sim65 reported it encountered the JMP-indirect 6502 bug at address 0x%04x.\n", filename, testcase_index, testcase->name, cpu_state_ptr->PC);
        ++notices_seen;
    }

    if (sim65_mem_read_byte_address_violation)
    {
        printf("[%s:%u (\"%s\")] ERROR - sim65 tried to read memory beyond the last address.\n", filename, testcase_index, testcase->name);
        ++errors_seen;
    }

    if (sim65_reported_error)
    {
        printf("[%s:%u (\"%s\")] ERROR - A register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.a, cpu_state_ptr->AC);
        ++errors_seen;
    }

    if (sim65_reported_warning)
    {
        printf("[%s:%u (\"%s\")] ERROR - A register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.a, cpu_state_ptr->AC);
        ++errors_seen;
    }

    if (testcase->final_state.a != cpu_state_ptr->AC)
    {
        printf("[%s:%u (\"%s\")] ERROR - A register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.a, cpu_state_ptr->AC);
        ++errors_seen;
    }

    if (testcase->final_state.x != cpu_state_ptr->XR)
    {
        printf("[%s:%u (\"%s\")] ERROR - X register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.a, cpu_state_ptr->XR);
        ++errors_seen;
    }

    if (testcase->final_state.y != cpu_state_ptr->YR)
    {
        printf("[%s:%u (\"%s\")] ERROR - Y register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.a, cpu_state_ptr->YR);
        ++errors_seen;
    }

    if (testcase->final_state.p != cpu_state_ptr->SR)
    {
        printf("[%s:%u (\"%s\")] ERROR - P register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.p, cpu_state_ptr->SR);
        ++errors_seen;
    }

    if (testcase->final_state.s != cpu_state_ptr->SP)
    {
        printf("[%s:%u (\"%s\")] ERROR - S register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase_index, testcase->name, testcase->final_state.s, cpu_state_ptr->SP);
        ++errors_seen;
    }

    if (testcase->final_state.pc != cpu_state_ptr->PC)
    {
        printf("[%s:%u (\"%s\")] ERROR - PC register check failed (expected: 0x%04x, sim65: 0x%04x).\n", filename, testcase_index, testcase->name, testcase->final_state.s, cpu_state_ptr->PC);
        ++errors_seen;
    }

    if (testcase->cycles != sim65_cyclecount)
    {
        printf("[%s:%u (\"%s\")] ERROR - cycle count check failed (expected: %u, sim65: %u).\n", filename, testcase_index, testcase->name, testcase->cycles, sim65_cyclecount);
        ++errors_seen;
    }

    if (memcmp(testcase->final_state.ram, mem, 0x10000) != 0)
    {
        printf("[%s:%u (\"%s\")] ERROR - memory check failed.\n", filename, testcase_index, testcase->name);
        ++errors_seen;
    }

    printf("[%s:%u (\"%s\")] INFO - Test summary: %u %s, %u %s.\n", filename, testcase_index, testcase->name,
            errors_seen, (errors_seen != 1) ? "errors" : "error",
            notices_seen, (notices_seen != 1) ? "notices" : "notice");

    return errors_seen ? -1 : 0;
}
