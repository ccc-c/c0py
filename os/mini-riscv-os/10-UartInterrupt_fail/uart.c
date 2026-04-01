#include "uart.h"

void uart_init() {
    volatile uint8_t* uart_ier = (volatile uint8_t*)UART_IER;
    volatile uint8_t* uart_lcr = (volatile uint8_t*)UART_LCR;
    volatile uint8_t* uart_fcr = (volatile uint8_t*)UART_FCR;
    volatile uint8_t* uart_rbr = (volatile uint8_t*)UART_RBR;
    volatile uint8_t* uart_lsr = (volatile uint8_t*)UART_LSR;
    
    // Disable interrupts first
    *uart_ier = 0x00;
    
    // Set DLAB=1 to access divisor
    *uart_lcr = 0x80;
    
    // Set divisor to 115200 baud (for QEMU, this is don't care)
    *uart_rbr = 12;
    *(uart_ier + 1) = 0x00;
    
    // Set DLAB=0, 8N1 (8 bits, no parity, 1 stop)
    *uart_lcr = 0x03;
    
    // Enable FIFO (optional)
    *uart_fcr = 0x01;
    
    // Enable RX interrupt (IER bit 0)
    *uart_ier = UART_IER_RX;
    
    lib_printf("UART initialized\n");
}

void uart_isr() {
    volatile uint8_t* uart_rbr = (volatile uint8_t*)UART_RBR;
    volatile uint8_t* uart_lsr = (volatile uint8_t*)UART_LSR;
    uint8_t lsr = *uart_lsr;
    
    if (lsr & UART_LSR_DATA_READY) {
        uint8_t c = *uart_rbr;
        lib_printf("UART received: 0x%02x ('%c')\n", c, (c >= 32 && c < 127) ? c : '?');
    }
}

int uart_getc() {
    volatile uint8_t* uart_rbr = (volatile uint8_t*)UART_RBR;
    volatile uint8_t* uart_lsr = (volatile uint8_t*)UART_LSR;
    uint8_t lsr = *uart_lsr;
    
    if (lsr & UART_LSR_DATA_READY) {
        return *uart_rbr;
    }
    return -1;
}
