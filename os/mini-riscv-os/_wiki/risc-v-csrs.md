# RISC-V-CSRs

Control and Status Registers (CSRs) in RISC-V machine mode.

## Common Machine-Mode CSRs

| CSR | Name | Description |
|-----|------|-------------|
| mhartid | Hart ID | Hardware thread ID (read-only) |
| mstatus | Machine Status | Global interrupt enable, privilege mode |
| mie | Machine Interrupt Enable | Individual interrupt enable bits |
| mtvec | Machine Trap Vector | Address of trap/interrupt handler |
| mepc | Machine Exception PC | Return address after trap |
| mscratch | Machine Scratch | Scratch register for trap handler |
| mcause | Machine Cause | Reason for trap (interrupt/exception) |

## Key Bits in mstatus

- **MIE (bit 3)**: Machine Interrupt Enable
- **MPP (bits 11-12)**: Previous Privilege Mode

## Key Bits in mie

- **MTIE (bit 7)**: Machine Timer Interrupt Enable
- **MEIE (bit 11)**: Machine External Interrupt Enable
- **MSIE (bit 3)**: Machine Software Interrupt Enable

## Usage in mini-riscv-os

```c
// Read hart ID
int id = r_mhartid();

// Enable interrupts
w_mstatus(r_mstatus() | MSTATUS_MIE);

// Set trap vector
w_mtvec((reg_t)sys_timer);

// Enable timer interrupt
w_mie(r_mie() | MIE_MTIE);
```

## Related

- See also: [[04-Timer-Interrupt]], [[RISC-V-Registers]]
