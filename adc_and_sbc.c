
///////////////////
// adc_and_sbc.c //
///////////////////

#include "adc_and_sbc.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                              //
//                                           Binary mode: 6502 & 65C02 versions are identical                                   //
//                                                                                                                              //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline struct OpResult adc_binary_mode(const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    struct OpResult result;

    const bool initial_accumulator_negative = (initial_accumulator >= 0x80);
    const bool operand_negative = (operand >= 0x80);

    result.Accumulator = (initial_carry_flag + initial_accumulator + operand);
    result.FlagN = (result.Accumulator >= 0x80);
    result.FlagV = (initial_accumulator_negative ^ result.FlagN) & (operand_negative ^ result.FlagN);
    result.FlagZ = result.Accumulator == 0;
    result.FlagC = (initial_carry_flag + initial_accumulator + operand) >= 0x100;

    return result;
}

static inline struct OpResult sbc_binary_mode(const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    // SBC in binary mode is identical to the ADC in binary mode with the operand inverted.

    struct OpResult result;

    const bool initial_accumulator_negative = (initial_accumulator >= 0x80);
    const bool operand_nonnegative = (operand < 0x80);

    const bool borrow = !initial_carry_flag;

    result.Accumulator = initial_accumulator  - operand - borrow;
    result.FlagN = (result.Accumulator & 0x80) != 0;
    result.FlagV = (initial_accumulator_negative ^ result.FlagN) & (operand_nonnegative ^ result.FlagN);
    result.FlagZ = result.Accumulator == 0;
    result.FlagC = initial_accumulator >= operand + borrow;

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                              //
//                                                         6502-specific versions                                               //
//                                                                                                                              //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline struct OpResult adc_6502_decimal_mode(const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    struct OpResult result;

    const bool initial_accumulator_negative = (initial_accumulator & 0x80) != 0;
    const bool operand_negative = (operand & 0x80) != 0;

    // For the 6502 ADC instruction in decimal mode, the Z flag behaves as if we're in binary mode.

    const uint8_t binary_result = (initial_carry_flag + initial_accumulator + operand);
    result.FlagZ = (binary_result == 0);

    // For the 6502 ADC instruction in decimal mode, the Accumulator and the N, V, and C flags behave differently.

    bool carry = initial_carry_flag;

    uint8_t low_nibble = (initial_accumulator & 15) + (operand & 15) + carry;
    if ((carry = low_nibble > 9))
    {
        low_nibble = (low_nibble - 10) & 15;
    }

    uint8_t high_nibble = (initial_accumulator >> 4) + (operand >> 4) + carry;

    // For ADC, the N and V flags are determined based on the high nibble calculated before carry-correction.
    result.FlagN = (high_nibble & 8) != 0;
    result.FlagV = (initial_accumulator_negative ^ result.FlagN) & (operand_negative ^ result.FlagN);

    if ((carry = high_nibble > 9))
    {
        high_nibble = (high_nibble - 10) & 15;
    }

    result.Accumulator = (high_nibble << 4) | low_nibble;
    result.FlagC = carry;

    return result;
}

static inline struct OpResult sbc_6502_decimal_mode(const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    struct OpResult result;

    // For the 6502 SBC instruction in decimal mode, the N, V, and Z flags behave as if we're in binary mode.

    const bool initial_accumulator_negative = (initial_accumulator & 0x80) != 0;
    const bool operand_nonnegative = (operand & 0x80) == 0;

    bool borrow = !initial_carry_flag;

    const uint8_t binary_result = initial_accumulator  - operand - borrow;
    result.FlagN = (binary_result & 0x80) != 0;
    result.FlagV = (initial_accumulator_negative ^ result.FlagN) & (operand_nonnegative ^ result.FlagN);
    result.FlagZ = (binary_result == 0);

    // For the 6502 SBC instruction in decimal mode, the Accumulator and the C flag behave differently.

    uint8_t low_nibble = (initial_accumulator & 15) - (operand & 15) - borrow;
    if ((borrow = ((low_nibble & 0x80) != 0)))
    {
        low_nibble = (low_nibble + 10) & 15;
    }

    uint8_t high_nibble = (initial_accumulator >> 4) - (operand >> 4) - borrow;
    if ((borrow = ((high_nibble & 0x80) != 0)))
    {
        high_nibble = (high_nibble + 10) & 15;
    }

    result.Accumulator = (high_nibble << 4) | low_nibble;
    result.FlagC = !borrow;

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                              //
//                                                     65C02-specific versions                                                  //
//                                                                                                                              //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline struct OpResult adc_65c02_decimal_mode(const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    struct OpResult result;

    bool carry = initial_carry_flag;

    uint8_t low_nibble = (initial_accumulator & 15) + (operand  & 15) + carry;
    if ((carry = low_nibble > 9))
    {
        low_nibble -= 10;
    }
    low_nibble &= 15;

    uint8_t high_nibble = (initial_accumulator >> 4) + (operand  >> 4) + carry;

    uint8_t xn = (high_nibble & 8) != 0; // Used for Overflow flag.

    if ((carry = high_nibble > 9))
    {
        high_nibble -= 10;
    }
    high_nibble &= 15;

    result.Accumulator = (high_nibble << 4) | low_nibble;
    result.FlagN = (result.Accumulator >= 0x80);
    result.FlagV = ((initial_accumulator >= 0x80) ^ xn) & ((operand >= 0x80) ^ xn);
    result.FlagZ = (result.Accumulator == 0x00);
    result.FlagC = carry;

    return result;
}

static inline struct OpResult sbc_65c02_decimal_mode(const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    struct OpResult result;

    bool borrow = !initial_carry_flag;

    uint8_t low_nibble = (initial_accumulator & 15) - (operand & 15) - borrow;
    if ((borrow = (low_nibble >= 0x80)))
    {
        low_nibble += 10;
    }
    // low_nibble still being negative here, strangely, influences the high nibble.
    const bool low_nibble_still_negative = (low_nibble >= 0x80);
    low_nibble &= 15;

    uint8_t high_nibble = (initial_accumulator >> 4) - (operand >> 4) - borrow;
    const bool high_nibble_msb = (high_nibble & 0x08) != 0; // Used for the Overflow flag later on.

    if ((borrow = (high_nibble >= 0x80)))
    {
        high_nibble += 10;
    }
    high_nibble -= low_nibble_still_negative;
    high_nibble &= 15;

    result.Accumulator = (high_nibble << 4) | low_nibble;
    result.FlagN = (result.Accumulator >= 0x80);
    result.FlagV = ((initial_accumulator >= 0x80) ^ high_nibble_msb) & ((operand < 0x80) ^ high_nibble_msb);
    result.FlagZ = (result.Accumulator == 0x00);
    result.FlagC = !borrow;

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                              //
//                                                          Entry Points                                                        //
//                                                                                                                              //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct OpResult adc_6502(const bool decimal_flag, const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    return decimal_flag ? adc_6502_decimal_mode(initial_carry_flag, initial_accumulator, operand)
                        : adc_binary_mode      (initial_carry_flag, initial_accumulator, operand);
}

struct OpResult sbc_6502(const bool decimal_flag, const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    return decimal_flag ? sbc_6502_decimal_mode(initial_carry_flag, initial_accumulator, operand)
                        : sbc_binary_mode      (initial_carry_flag, initial_accumulator, operand);
}

struct OpResult adc_65c02(const bool decimal_flag, const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    return decimal_flag ? adc_65c02_decimal_mode(initial_carry_flag, initial_accumulator, operand)
                        : adc_binary_mode       (initial_carry_flag, initial_accumulator, operand);
}

struct OpResult sbc_65c02(const bool decimal_flag, const bool initial_carry_flag, const uint8_t initial_accumulator, const uint8_t operand)
{
    return decimal_flag ? sbc_65c02_decimal_mode(initial_carry_flag, initial_accumulator, operand)
                        : sbc_binary_mode       (initial_carry_flag, initial_accumulator, operand);
}
