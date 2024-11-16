
#ifndef PARAVIRT_H
#define PARAVIRT_H

#include "6502.h"

// For sim65-test, this function is implemented in sim65-testcase.c.
void ParaVirtHooks (CPURegs* Regs);

#endif
