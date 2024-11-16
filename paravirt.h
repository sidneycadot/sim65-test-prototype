
// NOTE: This is a replacement for sim65's "paravirt.h", providing only the definitions needed to support sim65-test.

#ifndef PARAVIRT_H
#define PARAVIRT_H

#include "6502.h"

// For sim65-test, this function is implemented in sim65-testcase.c.
void ParaVirtHooks (CPURegs* Regs);

#endif
