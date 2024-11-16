
//////////////////
// sim65-test.c //
//////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "sim65-testcase.h"

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

int parse_json_testcase(cJSON * json_testcase, struct sim65_testcase_specification_type * testcase)
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
            int testcase_result = execute_testcase(&testcase, filename, testcase_index);
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

    if (argc == 1)
    {
        puts("Usage: sim65-test [FILE]...");
        puts("");
        puts("Test the instruction execution code from sim65 using JSON-formatted testcases.");
        puts("");
        puts("Each FILE should be a JSON-formatted file specifying single-instruction testcases,");
        puts("formatted according to the conventions used in the 65x02 project.");
        puts("");
        puts("The precise JSON format specification, as welll as a large corpus of testcases,");
        puts("can be obtained from the 65x02 project:");
        puts("");
        puts("  https://github.com/SingleStepTests/65x02");
        puts("");
    }

    for (int i = 1; i < argc; ++i)
    {
        process_testcase_file(argv[i]);
    }

    return 0;
}
