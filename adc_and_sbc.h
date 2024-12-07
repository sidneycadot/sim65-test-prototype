
///////////////////
// adc_and_sbc.h //
///////////////////

#ifndef ADC_AND_SBC_H
#define ADC_AND_SBC_H

#include <stdbool.h>
#include <stdint.h>

struct OpResult
{
    uint8_t  Accumulator;
    bool     FlagN;
    bool     FlagV;
    bool     FlagZ;
    bool     FlagC;
};

// 6502 versions of ADC and SBC.
// Verified to produce bitwise identical results to a hardware 6502 for all inputs.
// Verification was done using an Atari 800XL, which has a SALLY 6502 chip.

struct OpResult adc_6502(const bool decimal_flag, const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand);
struct OpResult sbc_6502(const bool decimal_flag, const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand);

// 65C02 versions of ADC and SBC.
// Verified to produce bitwise identical results to a hardware 65C02 for all inputs.
// Verification was done using a Neo6502 board, which has a WDC 65C02 chip.

struct OpResult adc_65c02(const bool decimal_flag, const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand);
struct OpResult sbc_65c02(const bool decimal_flag, const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand);

#endif
