#include "plic.h"
#include "lib.h"
#include "isr.h"

void plic_init() {
    int hart = r_mhartid();
    
    // Set threshold to 0 (accept all interrupts)
    plic_set_threshold(0);
    
    lib_printf("PLIC initialized for hart %d\n", hart);
}

void plic_enable_irq(int irq, int priority) {
    // Set priority for this IRQ
    plic_set_priority(irq, priority);
    
    // Enable this IRQ
    plic_enable(irq);
    
    lib_printf("PLIC enabled IRQ %d with priority %d\n", irq, priority);
}
