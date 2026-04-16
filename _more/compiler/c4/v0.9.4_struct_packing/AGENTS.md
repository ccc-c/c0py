# AGENTS.md - C4 (C in Four Functions)

C4 is a self-compiling C interpreter (C → bytecode → stack-based VM).

## Build
```bash
gcc -w -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined c4.c -o c4
```

## Run
```bash
./c4 <source.c>      # compile and run
./c4 -s <source.c>   # -s: show source with bytecode output
./c4 -d <source.c>    # -d: debug mode
```

## Self-compile
```bash
./c4 c4.c hello.c         # compile c4.c with hello.c as input source
./c4 c4.c c4.c hello.c    # bootstrap: c4 compiles itself
```

## Test
```bash
./test.sh   # runs ./c4 -s test/fib.c
```

Test files in `test/`: fib.c, sum.c, sum_for.c, float.c, init.c, error.c, comment.c, arg.c, var.c, hello.c

## Language support
- functions, structs, pointers, floats/doubles, arrays
- Local variables must be declared at function start (C89 style)
- `#include` lines are ignored (kept for compatibility)
- `argc`/`argv` supported in `main()`

## Architecture
- Bytecode VM: 30+ instructions, stack-based (pc/bp/sp/a registers)
- ELF-style section offsets for text/data (no absolute pointers)
- Float extensions: FLI, FLD, FST, FSD, FADD, FSUB, FMUL, FDIV, FEQ, FNE, FLT, FGT, FLE, FGE, ITF, FTI
- Syscalls: OPEN, READ, CLOS, PRTF, MALC, FREE, MSET, MCMP, EXIT

See `_doc/vm.md` (integer ops) and `_doc/vm_float.md` (float ops).
