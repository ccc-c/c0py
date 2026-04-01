#ifndef __UART_H__
#define __UART_H__

#include "riscv.h"
#include "lib.h"

void uart_init();
void uart_isr();
int uart_getc();

#endif
