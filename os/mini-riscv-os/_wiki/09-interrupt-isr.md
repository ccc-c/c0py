# 09-Interrupt-ISR

The ninth stage implements a complete Interrupt Service Routine (ISR) registration and dispatch system.

## Key Concepts

- **ISR Table**: Array mapping IRQ numbers to handler functions
- **Dynamic Registration**: Tasks can register handlers at runtime
- **IRQ Dispatch**: Central handler dispatches to registered ISR
- **Interrupt Handling**: Generic framework for various interrupt sources

## Implementation Details

### ISR Structure

```c
typedef struct {
    int irq;
    void (*handler)(void);
} isr_entry_t;

#define MAX_ISR 16

static isr_entry_t isr_table[MAX_ISR];
```

### ISR Initialization

```c
void isr_init() {
    for (int i = 0; i < MAX_ISR; i++) {
        isr_table[i].irq = -1;
        isr_table[i].handler = 0;
    }
}
```

### ISR Registration

```c
int isr_register(int irq, isr_handler_t handler) {
    if (irq < 0 || irq >= MAX_ISR) {
        return -1;
    }
    isr_table[irq].irq = irq;
    isr_table[irq].handler = handler;
    return 0;
}
```

### ISR Dispatch

```c
void isr_dispatch(int irq) {
    if (irq >= 0 && irq < MAX_ISR) {
        if (isr_table[irq].handler != 0) {
            isr_table[irq].handler();
        }
    }
}
```

### User Task with ISR

```c
volatile int isr_call_count = 0;

void my_timer_isr(void) {
    isr_call_count++;
}

void user_task4(void) {
    lib_printf("Task4: registering ISR for IRQ 5...\n");
    isr_register(5, my_timer_isr);
    lib_printf("Task4: ISR registered, call_count=%d\n", isr_call_count);
    
    while (1) {
        if (isr_call_count != last_count) {
            lib_printf("Task4 [hart %d]: ISR called! count=%d\n", r_mhartid(), isr_call_count);
            last_count = isr_call_count;
        }
        // ...
    }
}
```

## Build & Run

```sh
make
make qemu
# Output:
# OS start
# Task0 [hart 0]: Created!
# ...
# Task4: registering ISR for IRQ 5...
# Task4: ISR registered, call_count=0
# Task4 [hart 0]: ISR called! count=1
# Task4 [hart 0]: counter before = 1
# Task4 [hart 0]: counter after  = 2
# ...
```

## Key Takeaways

1. **Generic handler**: ISR table allows any function to handle any IRQ
2. **Runtime registration**: Handlers registered at runtime, not compile time
3. **IRQ dispatch**: Central dispatcher calls appropriate handler
4. **Use cases**: Timer, UART, external devices

## Related

- Previous: [[08-Multicore-64bit]]
- See also: [[ISR-Design]], [[Interrupt-Dispatch]]
