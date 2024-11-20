
// NOTE: This is a replacement for sim65's "memory.h", providing only the definitions needed to support sim65-test.

#ifndef MEMORY_H
#define MEMORY_H

#if (FIX_SIM65==1)

#include <inttypes.h>
void MemWriteByte (uint16_t Addr, uint8_t Val);
void MemWriteWord (uint16_t Addr, uint16_t Val);
uint8_t MemReadByte (uint16_t Addr);
uint16_t MemReadWord (uint16_t Addr);
uint16_t MemReadZPWord (uint8_t Addr);
#else
void MemWriteByte (unsigned Addr, unsigned char Val);
void MemWriteWord (unsigned Addr, unsigned Val);
unsigned char MemReadByte (unsigned Addr);
unsigned MemReadWord (unsigned Addr);
unsigned MemReadZPWord (unsigned char Addr);
#endif

#endif
