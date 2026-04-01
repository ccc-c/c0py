#ifndef __RISCV_H__
#define __RISCV_H__

#include <stdint.h>

// #define reg_t uint32_t // RISCV32: register is 32bits
#define reg_t uint64_t // RISCV64: register is 64bits

// ref: https://www.activexperts.com/serial-port-component/tutorials/uart/
#define UART        0x10000000
#define UART_RBR    (uint8_t*)(UART+0x00) // RBR:Receiver Buffer Register (read)
#define UART_THR    (uint8_t*)(UART+0x00) // THR:Transmitter Holding Register (write)
#define UART_IER    (uint8_t*)(UART+0x01) // IER:Interrupt Enable Register
#define UART_IIR    (uint8_t*)(UART+0x02) // IIR:Interrupt Identification Register
#define UART_FCR    (uint8_t*)(UART+0x02) // FCR:FIFO Control Register (write)
#define UART_LCR    (uint8_t*)(UART+0x03) // LCR:Line Control Register
#define UART_MCR    (uint8_t*)(UART+0x04) // MCR:Modem Control Register
#define UART_LSR    (uint8_t*)(UART+0x05) // LSR:Line Status Register
#define UART_MSR    (uint8_t*)(UART+0x06) // MSR:Modem Status Register
#define UART_SCR    (uint8_t*)(UART+0x07) // SCR:Scratch Register

// UART LSR bits
#define UART_LSR_DATA_READY  0x01  // Data Ready
#define UART_LSR_OVERRUN     0x02  // Overrun Error
#define UART_LSR_PARITY      0x04  // Parity Error
#define UART_LSR_FRAMING     0x08  // Framing Error
#define UART_LSR_BREAK       0x10  // Break Interrupt
#define UART_LSR_TX_EMPTY   0x20  // Transmitter Empty
#define UART_LSR_TX_IDLE    0x40  // Transmitter Idle

// UART IER bits
#define UART_IER_RX         0x01  // Received Data Available Interrupt
#define UART_IER_TX         0x02  // Transmitter Holding Register Empty Interrupt
#define UART_IER_LINE       0x04  // Line Status Interrupt
#define UART_IER_MODEM     0x08  // Modem Status Interrupt

// UART IRQ numbers (from PLIC)
#define UART0_IRQ 10

// Saved registers for kernel context switches.
struct context {
  reg_t ra;
  reg_t sp;

  // callee-saved
  reg_t s0;
  reg_t s1;
  reg_t s2;
  reg_t s3;
  reg_t s4;
  reg_t s5;
  reg_t s6;
  reg_t s7;
  reg_t s8;
  reg_t s9;
  reg_t s10;
  reg_t s11;
};

// ref: https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/riscv.h
// 
// local interrupt controller, which contains the timer.
// ================== Timer Interrput ====================

#define NCPU 8             // maximum number of CPUs
#define CLINT 0x2000000
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

// which hart (core) is this?
static inline reg_t r_mhartid()
{
  reg_t x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

// Machine Status Register, mstatus
#define MSTATUS_MPP_MASK (3UL << 11) // previous mode.
#define MSTATUS_MPP_M (3UL << 11)
#define MSTATUS_MPP_S (1UL << 11)
#define MSTATUS_MPP_U (0UL << 11)
#define MSTATUS_MIE (1UL << 3)    // machine-mode interrupt enable.

static inline reg_t r_mstatus()
{
  reg_t x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void w_mstatus(reg_t x)
{
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void w_mepc(reg_t x)
{
  asm volatile("csrw mepc, %0" : : "r" (x));
}

static inline reg_t r_mepc()
{
  reg_t x;
  asm volatile("csrr %0, mepc" : "=r" (x));
  return x;
}

// Machine Scratch register, for early trap handler
static inline void w_mscratch(reg_t x)
{
  asm volatile("csrw mscratch, %0" : : "r" (x));
}

// Machine-mode interrupt vector
static inline void w_mtvec(reg_t x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}

// Machine-mode Interrupt Enable
#define MIE_MEIE (1UL << 11) // external
#define MIE_MTIE (1UL << 7)  // timer
#define MIE_MSIE (1UL << 3)  // software

// PLIC (Platform Level Interrupt Controller)
#define PLIC 0x0c000000
#define PLIC_PRIORITY(i) (PLIC + (i) * 4)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_ENABLE(hart) (PLIC + 0x2000 + (hart) * 0x80)
#define PLIC_THRESHOLD(hart) (PLIC + 0x201000 + (hart) * 0x1000)
#define PLIC_CLAIM(hart) (PLIC + 0x201004 + (hart) * 0x1000)

// UART IRQ numbers (from PLIC)
#define UART0_IRQ 10

static inline reg_t r_mie()
{
  reg_t x;
  asm volatile("csrr %0, mie" : "=r" (x) );
  return x;
}

static inline void w_mie(reg_t x)
{
  asm volatile("csrw mie, %0" : : "r" (x));
}

// PLIC functions
static inline reg_t plic_claim() {
    reg_t x;
    int hart = r_mhartid();
    asm volatile("lw %0, 0(%1)" : "=r" (x) : "r" (PLIC_CLAIM(hart)));
    return x;
}

static inline void plic_complete(reg_t irq) {
    int hart = r_mhartid();
    asm volatile("sw %0, 0(%1)" : : "r" (irq), "r" (PLIC_CLAIM(hart)));
}

static inline void plic_set_threshold(int threshold) {
    int hart = r_mhartid();
    asm volatile("sw %0, 0(%1)" : : "r" (threshold), "r" ((uintptr_t)PLIC_THRESHOLD(hart)));
}

static inline void plic_set_priority(int irq, int priority) {
    asm volatile("sw %0, 0(%1)" : : "r" (priority), "r" ((uintptr_t)PLIC_PRIORITY(irq)));
}

static inline void plic_enable(int irq) {
    int hart = r_mhartid();
    uint32_t val;
    asm volatile("lw %0, 0(%1)" : "=r" (val) : "r" ((uintptr_t)PLIC_ENABLE(hart)));
    val |= (1 << irq);
    asm volatile("sw %0, 0(%1)" : : "r" (val), "r" ((uintptr_t)PLIC_ENABLE(hart)));
}

#endif