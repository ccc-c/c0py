# RISC-V-Registers

RISC-V register conventions and calling conventions.

## General Purpose Registers

| Register | ABI Name | Description | Saved by |
|----------|----------|-------------|----------|
| x0 | zero | Constant 0 | - |
| x1 | ra | Return address | Caller |
| x2 | sp | Stack pointer | Callee |
| x3 | gp | Global pointer | - |
| x4 | tp | Thread pointer | - |
| x5-x7 | t0-t2 | Temporaries | Caller |
| x8-x9 | s0-s1 | Saved registers | Callee |
| x10-x17 | a0-a7 | Function arguments | Caller |
| x18-x27 | s2-s11 | Saved registers | Callee |
| x28-x31 | t3-t6 | Temporaries | Caller |

## Calling Convention

- **Caller-saved** (t0-t6, a0-a7): Function can clobber
- **Callee-saved** (s0-s11): Function must preserve

## Context Switching

For context switch, save these callee-saved registers:
- ra (return address)
- sp (stack pointer)
- s0-s11 (saved registers)

## Related

- See also: [[02-Context-Switch]], [[RISC-V-CSRs]]
