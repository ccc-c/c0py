# 10-UartInterrupt (UART + PLIC External Interrupts)

## Features
- Timer interrupt (CLINT) - drives preemptive multitasking
- UART interrupt (NS16550) via PLIC - receives characters from QEMU
- PLIC (Platform Level Interrupt Controller) for external interrupts
- Multi-core support (4 harts)

## Build & Run

```sh
make clean
make
make qemu
```

## Testing UART Interrupt
1. Run QEMU with: `make qemu`
2. Type characters in the QEMU terminal
3. Task4 will receive UART interrupts and print the characters
