
// NOTE: This is a replacement for sim65's "memory.h", providing only the definitions needed to support sim65-test.

#ifndef MEMORY_H
#define MEMORY_H

void MemWriteByte (unsigned Addr, unsigned char Val);

void MemWriteWord (unsigned Addr, unsigned Val);

unsigned char MemReadByte (unsigned Addr);

unsigned MemReadWord (unsigned Addr);

unsigned MemReadZPWord (unsigned char Addr);

#endif
