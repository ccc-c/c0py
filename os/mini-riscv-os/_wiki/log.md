# Log

## [2026-04-08] Create mini-riscv-os wiki

### Ingest

- Created `_wiki/` directory in mini-riscv-os project
- Reviewed 9 lesson directories: 01-HelloOs through 09-InterruptISR

### Wiki Pages Created

1. `index.md` - Overview and lesson index
2. `01-hello-os.md` - Boot process, UART, stack setup
3. `02-context-switch.md` - Manual context switching
4. `03-multi-tasking.md` - Cooperative multitasking
5. `04-timer-interrupt.md` - CLINT timer interrupts
6. `05-preemptive.md` - Preemptive scheduling
7. `06-mutex.md` - Mutual exclusion primitives
8. `07-multicore-32bit.md` - 32-bit multicore support
9. `08-multicore-64bit.md` - 64-bit RISC-V (rv64ima, LP64)
10. `09-interrupt-isr.md` - ISR registration and dispatch system

### Key Conventions

- Wiki pages use `[[wikilinks]]` for cross-references
- Each lesson page includes: Key Concepts, Implementation Details, Build & Run, Key Takeaways
- Topics use kebab-case (e.g., `01-hello-os`)
- Related sections link to previous/next lessons and concept pages

### Pattern Applied

- Followed LLM Wiki pattern from https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f
- Created index.md for navigation
- Each page is a self-contained lesson summary
