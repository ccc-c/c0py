# Mini-RISC-V OS Wiki

A step-by-step educational operating system built from scratch for RISC-V architecture.

## Overview

This project implements a minimal operating system for RISC-V, progressing through 9 stages from basic "Hello OS" to multicore 64-bit systems with interrupt handling.

## Lessons

| Lesson | Topic | Description |
|--------|-------|-------------|
| [[01-hello-os]] | Hello OS | Basic boot and UART output |
| [[02-context-switch]] | Context Switch | Manual task switching via assembly |
| [[03-multi-tasking]] | Multi-Tasking | Cooperative multitasking kernel |
| [[04-timer-interrupt]] | Timer Interrupt | Timer interrupts using CLINT |
| [[05-preemptive]] | Preemptive | Time-sliced preemptive scheduling |
| [[06-mutex]] | Mutex | Mutual exclusion for shared resources |
| [[07-multicore-32bit]] | Multicore 32-bit | Multi-core support (32-bit) |
| [[08-multicore-64bit]] | Multicore 64-bit | 64-bit RISC-V multicore |
| [[09-interrupt-isr]] | Interrupt ISR | Interrupt Service Routine system |

## Concepts

| Concept | Description |
|---------|-------------|
| [[risc-v-csrs]] | RISC-V Control and Status Registers |
| [[risc-v-registers]] | RISC-V Register Conventions |

## Architecture

- **ISA**: RISC-V 32-bit (rv32ima) / 64-bit (rv64ima)
- **ABI**: ILP32 (32-bit) / LP64 (64-bit)
- **Simulation**: QEMU riscv32/riscv64 virt machine

## Related

- Source: [mini-riscv-os repository](../)
- Build: `make` to compile, `make qemu` to run
