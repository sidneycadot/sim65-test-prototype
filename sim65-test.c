
//////////////////
// sim65-test.c //
//////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "sim65-testcase.h"

static int parse_json_u8_field(cJSON * json_object, char * field_name, uint8_t * value)
{
    cJSON * field = cJSON_GetObjectItemCaseSensitive(json_object, field_name);
    if (!cJSON_IsNumber(field))
    {
        return -1; // Field not found.
    }

    if (field->valueint < 0 || field->valueint > 0xff)
    {
        return -1; // Expecting an unsigned 8-bit value.
    }

    *value = field->valueint;

    return 0; // Success.
}

static int parse_json_u16_field(cJSON * json_object, char * field_name, uint16_t * value)
{
    cJSON * field = cJSON_GetObjectItemCaseSensitive(json_object, field_name);
    if (!cJSON_IsNumber(field))
    {
        return -1; // Field not found.
    }

    if (field->valueint < 0 || field->valueint > 0xffff)
    {
        return -1; // Expecting an unsigned 16-bit value.
    }

    *value = field->valueint;

    return 0; // Success.
}

static int parse_json_machine_state_field(cJSON * json_testcase, char * field_name, struct machine_state_type * state)
{
    if (!cJSON_IsObject(json_testcase))
    {
        return -1;
    }

    cJSON * json_field = cJSON_GetObjectItemCaseSensitive(json_testcase, field_name);

    if (parse_json_u16_field(json_field, "pc", &state->pc) != 0 ||
        parse_json_u8_field (json_field, "s" , &state->s ) != 0 ||
        parse_json_u8_field (json_field, "a" , &state->a ) != 0 ||
        parse_json_u8_field (json_field, "x" , &state->x ) != 0 ||
        parse_json_u8_field (json_field, "y" , &state->y ) != 0 ||
        parse_json_u8_field (json_field, "p" , &state->p ) != 0)
    {
        return -1;
    }

    cJSON * ramspec = cJSON_GetObjectItemCaseSensitive(json_field, "ram");
    if (!cJSON_IsArray(ramspec))
    {
        return -1;
    }

    // Initialize RAM to zero.
    memset(state->ram, 0, 0x10000);

    // Perform the assignments specified in the JSON file.
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

static int parse_json_testcase(cJSON * json_testcase, struct sim65_testcase_specification_type * testcase)
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

static int process_json_testcase_array(const char * filename, cJSON * json_testcase_array, enum sim65_cpu_mode_type cpu_mode, unsigned test_flags)
{
    if (!cJSON_IsArray(json_testcase_array))
    {
        return -1; // We expect an array.
    }

    unsigned testcase_index = 0; // First testcase will be 1, and so on.
    unsigned testcase_error = 0;

    for (cJSON * json_testcase = json_testcase_array->child; json_testcase != NULL; json_testcase = json_testcase->next)
    {
        ++testcase_index;

        struct sim65_testcase_specification_type testcase;

        int result = parse_json_testcase(json_testcase, &testcase);
        if (result != 0)
        {
            printf("[%s:%u] ERROR: Testcase cannot be parsed.\n", filename, testcase_index);
        }
        else
        {
            int testcase_result = execute_testcase(&testcase, filename, testcase_index, cpu_mode, test_flags);
            if (testcase_result != 0)
            {
                ++testcase_error;
            }
        }
    }

    printf("[%s] INFO - Test file summary: %u of %u testcases show deviations from expected behavior.\n",
           filename, testcase_error, testcase_index);

    return 0;
}

static int process_testcase_file(char * filename, enum sim65_cpu_mode_type cpu_mode, unsigned test_flags)
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
        return -1; // fseek() to end error.
    }

    long ftell_result = ftell(f);
    if (ftell_result < 0)
    {
        fclose(f);
        return -1; // ftell() error.
    }

    size_t string_size = ftell_result;

    char *string = malloc(string_size);
    if (string == NULL)
    {
        fclose(f);
        return -1; // malloc() error.
    }

    int fseek_end_result = fseek(f, 0, SEEK_SET);
    if (fseek_end_result != 0)
    {
        free(string);
        fclose(f);
        return -1; // fseek() to beginning error.
    }

    size_t number_of_items = fread(string, 1, string_size, f);
    if (number_of_items != string_size)
    {
        free(string);
        fclose(f);
        return -1; // fread() error.
    }

    int fclose_result = fclose(f);
    if (fclose_result != 0)
    {
        free(string);
        return -1; // close() error.
    }

    cJSON *json_testcase_array = cJSON_ParseWithLength(string, string_size);
    free(string);

    if (!cJSON_IsArray(json_testcase_array))
    {
        return -1; // JSON parse error.
    }

    int result = process_json_testcase_array(filename, json_testcase_array, cpu_mode, test_flags);

    cJSON_Delete(json_testcase_array);

    return result;
}

void print_help(void)
{
    puts("Usage: sim65-test [--cpu-mode=<mode>] [FILE]...");
    puts("");
    puts("Test the instruction execution code from sim65 using JSON-formatted test cases.");
    puts("");
    puts("Each FILE should be a JSON-formatted file specifying a number of test cases,");
    puts("each describing the effect of the execution of a single instruction.");
    puts("");
    puts("Test case files support the JSON-based convention used in the 65x02 project:");
    puts("");
    puts("    https://github.com/SingleStepTests/65x02");
    puts("");
    puts("The 65x02 test suite can be obtained by cloning the 65x02 Git repository:");
    puts("");
    puts("    git clone git@github.com:SingleStepTests/65x02.git");
    puts("");
    puts("By default, sim65-test will configure sim65 to simulate a vanilla '6502' CPU.");
    puts("This can be changed by providing a --cpu-mode=<mode> argument:");
    puts("");
    puts("  --cpu-mode=6502      Simulate a vanilla 6502 processor.");
    puts("  --cpu-mode=6502X     Simulate a 6502X processor.");
    puts("  --cpu-mode=65C02     Simulate a 65C02 processor.");
    puts("");}

int main(int argc, char ** argv)
{
    enum sim65_cpu_mode_type cpu_mode = SIM65_CPU_6502;
    unsigned test_flags = (F_TEST_CYCLECOUNT | F_TEST_MEMORY); // Enable all tests.

    if (argc == 1)
    {
        print_help();
        return EXIT_SUCCESS;
    }

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--help")==0)
        {
            print_help();
            return EXIT_SUCCESS;
        }
    }

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--cpu-mode=6502") == 0)
        {
            cpu_mode = SIM65_CPU_6502;
        }
        else if (strcmp(argv[i], "--cpu-mode=6502X") == 0)
        {
            cpu_mode = SIM65_CPU_6502X;
        }
        else if (strcmp(argv[i], "--cpu-mode=65C02") == 0)
        {
            cpu_mode = SIM65_CPU_65C02;
        }
        else if(strcmp(argv[i], "--disable-cycle-count-test") == 0)
        {
            test_flags &= ~F_TEST_CYCLECOUNT;
        }
        else if(strcmp(argv[i], "--disable-memory-test") == 0)
        {
            test_flags &= ~F_TEST_MEMORY;
        }
        else
        {
            int result = process_testcase_file(argv[i], cpu_mode, test_flags);
            if (result != 0)
            {
                printf("Unable to process file: %s\n", argv[i]);
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}
