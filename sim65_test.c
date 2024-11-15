
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "6502.h"
#include "memory.h"

#include "cJSON.h"

unsigned long long TotalCycles;
extern CPURegs Regs;

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

struct testcase_spec
{
    char * name;
    struct machine_state_type initial_state;
    struct machine_state_type final_state;
    unsigned cycles;
};

int execute_testcase(const char *filename, struct testcase_spec * testcase)
{
    // Initialize the sim65 state from the "initial" state.

    CPU = CPU_6502;

    Regs.AC = testcase->initial_state.a;
    Regs.XR = testcase->initial_state.x;
    Regs.YR = testcase->initial_state.y;
    Regs.ZR = 0;
    Regs.SR = testcase->initial_state.p;
    Regs.SP = testcase->initial_state.s;
    Regs.SP = testcase->initial_state.s;
    Regs.PC = testcase->initial_state.pc;

    memcpy(Mem, testcase->initial_state.ram, 0x10000);

    // Run a single instruction.
    TotalCycles = 0;
    unsigned cyclecount = ExecuteInsn();

    // Verify state of CPU and memory and cycle count.

    unsigned errors = 0;

    if (testcase->final_state.a != Regs.AC)
    {
        printf("[%s:%s] ERROR - A register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase->name, testcase->final_state.a, Regs.AC);
        ++errors;
    }

    if (testcase->final_state.x != Regs.XR)
    {
        printf("[%s:%s] ERROR - X register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase->name, testcase->final_state.a, Regs.XR);
        ++errors;
    }

    if (testcase->final_state.y != Regs.YR)
    {
        printf("[%s:%s] ERROR - Y register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase->name, testcase->final_state.a, Regs.YR);
        ++errors;
    }

    if (testcase->final_state.p != Regs.SR)
    {
        printf("[%s:%s] ERROR - P register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase->name, testcase->final_state.p, Regs.SR);
        ++errors;
    }

    if (testcase->final_state.s != Regs.SP)
    {
        printf("[%s:%s] ERROR - S register check failed (expected: 0x%02x, sim65: 0x%02x).\n", filename, testcase->name, testcase->final_state.s, Regs.SP);
        ++errors;
    }

    if (testcase->final_state.pc != Regs.PC)
    {
        printf("[%s:%s] ERROR - PC register check failed (expected: 0x%04x, sim65: 0x%04x).\n", filename, testcase->name, testcase->final_state.s, Regs.PC);
        ++errors;
    }

    if (testcase->cycles != cyclecount)
    {
        printf("[%s:%s] ERROR - cycle count check failed (expected: %u, sim65: %u).\n", filename, testcase->name, testcase->cycles, cyclecount);
        ++errors;
    }

    if (memcmp(testcase->final_state.ram, Mem, 0x10000) != 0)
    {
        printf("[%s:%s] ERROR - memory check failed.\n", filename, testcase->name);
        ++errors;
    }

    if (errors == 0)
    {
        printf("[%s:%s] SUCCESS.\n", filename, testcase->name);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int parse_json_ranged_unsigned_number_field(cJSON * json_object, char * field_name, unsigned max_unsigned_value, unsigned * value)
{
    cJSON * field = cJSON_GetObjectItemCaseSensitive(json_object, field_name);
    if (!cJSON_IsNumber(field))
    {
        return -1; // Field not found.
    }

    if (field->valueint < 0)
    {
        return -1; // Expecting an unsigned (non-negative) value.
    }

    unsigned unsigned_value = field->valueint;

    if (unsigned_value > max_unsigned_value)
    {
        return -1; // Number out of range.
    }

    *value = unsigned_value;
    return 0; // Success.
}

int parse_json_machine_state_field(cJSON * json_testcase, char * field_name, struct machine_state_type * state)
{
    if (!cJSON_IsObject(json_testcase))
    {
        return -1;
    }

    cJSON * json_field = cJSON_GetObjectItemCaseSensitive(json_testcase, field_name);

    if (parse_json_ranged_unsigned_number_field(json_field, "pc", 0xffff, &state->pc) != 0 ||
        parse_json_ranged_unsigned_number_field(json_field, "s" ,   0xff, &state->s ) != 0 ||
        parse_json_ranged_unsigned_number_field(json_field, "a" ,   0xff, &state->a ) != 0 ||
        parse_json_ranged_unsigned_number_field(json_field, "x" ,   0xff, &state->x ) != 0 ||
        parse_json_ranged_unsigned_number_field(json_field, "y" ,   0xff, &state->y ) != 0 ||
        parse_json_ranged_unsigned_number_field(json_field, "p" ,   0xff, &state->p ))
    {
        return -1;
    }

    cJSON * ramspec = cJSON_GetObjectItemCaseSensitive(json_field, "ram");
    if (!cJSON_IsArray(ramspec))
    {
        return -1;
    }

    memset(state->ram, 0, 0x10000);
    for (cJSON * assignment = ramspec->child; assignment != NULL; assignment = assignment->next)
    {
        if (!cJSON_IsArray(assignment))
        {
            return -1;
        }

        if (cJSON_GetArraySize(assignment) != 2)
        {
            return -1;
        }

        int address = assignment->child->valueint;
        if (address < 0 || address > 0xffff)
        {
            return -1;
        }

        int value = assignment->child->next->valueint;
        if (value < 0 || value > 0xff)
        {
            return -1;
        }

        state->ram[address] = value;
    }
    return 0;
}

int parse_json_testcase(cJSON * json_testcase, struct testcase_spec * testcase)
{
    if (!cJSON_IsObject(json_testcase))
    {
        return -1; // We expect an object.
    }

    cJSON * json_name = cJSON_GetObjectItemCaseSensitive(json_testcase, "name");
    if (!cJSON_IsString(json_name))
    {
        return -1;
    }
    testcase->name = json_name->valuestring;

    parse_json_machine_state_field(json_testcase, "initial", &testcase->initial_state);
    parse_json_machine_state_field(json_testcase, "final", &testcase->final_state);

    cJSON * json_cycles = cJSON_GetObjectItemCaseSensitive(json_testcase, "cycles");
    if (!cJSON_IsArray(json_cycles))
    {
        return -1; // Should be an array.
    }
    testcase->cycles = cJSON_GetArraySize(json_cycles);

    return 0;
}

int process_json_testcase_array(const char * filename, cJSON * json_testcase_array)
{
    if (!cJSON_IsArray(json_testcase_array))
    {
        return -1; // We expect an array.
    }

    for (cJSON * json_testcase = json_testcase_array->child; json_testcase != NULL; json_testcase = json_testcase->next)
    {
        struct testcase_spec testcase;

        int result = parse_json_testcase(json_testcase, &testcase);
        if (result != 0)
        {
            printf("[%s] ERROR: cannot parse testcase.\n", filename);
        }
        else
        {
            execute_testcase(filename, &testcase);
        }
    }

    return 0;
}

int process_testcase_file(char * filename)
{
    FILE * f = fopen(filename, "r");
    if (f == NULL)
    {
        return -1; // Cannot open file.
    }

    int fseek_begin_result = fseek(f, 0, SEEK_END);
    if (fseek_begin_result != 0)
    {
        fclose(f);
        return -2; // fseek() to end error.
    }

    long ftell_result = ftell(f);
    if (ftell_result < 0)
    {
        fclose(f);
        return -3; // ftell() error.
    }

    size_t string_size = ftell_result;

    char *string = malloc(string_size);
    if (string == NULL)
    {
        fclose(f);
        return -4; // malloc() error.
    }

    int fseek_end_result = fseek(f, 0, SEEK_SET);
    if (fseek_end_result != 0)
    {
        free(string);
        fclose(f);
        return -5; // fseek() to beginning error.
    }

    size_t number_of_items = fread(string, 1, string_size, f);
    if (number_of_items != string_size)
    {
        free(string);
        fclose(f);
        return -6; // fread() error.
    }

    int fclose_result = fclose(f);
    if (fclose_result != 0)
    {
        free(string);
        return -7; // close() error.
    }

    cJSON *json_testcase_array = cJSON_ParseWithLength(string, string_size);
    free(string);

    if (!cJSON_IsArray(json_testcase_array))
    {
        return -8; // JSON parse error.
    }

    int result = process_json_testcase_array(filename, json_testcase_array);

    cJSON_Delete(json_testcase_array);

    return result;
}

int main(int argc, char ** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        printf("testcase file: \"%s\"\n", argv[i]);
        process_testcase_file(argv[i]);
    }
    return 0;
}
