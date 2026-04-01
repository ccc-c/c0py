#ifndef __PLIC_H__
#define __PLIC_H__

#include "riscv.h"

void plic_init();
void plic_enable_irq(int irq, int priority);

#endif
