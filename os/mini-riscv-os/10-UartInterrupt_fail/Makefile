CC = riscv64-unknown-elf-gcc
CFLAGS = -nostdlib -fno-builtin -mcmodel=medany -march=rv64ima_zicsr -mabi=lp64

QEMU = qemu-system-riscv64
QFLAGS = -nographic -smp 4 -machine virt -bios none

all: os.elf

os.elf: start.s sys.s lib.c timer.c task.c mutex.c os.c user.c isr.c plic.c uart.c
	$(CC) $(CFLAGS) -T os.ld -o os.elf $^

qemu:
	@echo "Press Ctrl-A and then X to exit QEMU"
	$(QEMU) $(QFLAGS) -kernel os.elf

# UART interrupt test:
# Use two terminals:
#
# Terminal 1:
#   make qemu_serial
#   (note the PTY device shown, e.g., /dev/ttys015)
#
# Terminal 2:
#   sleep 5
#   echo "abc" > /dev/ttys015
#
# The characters should trigger UART RX interrupts

qemu_serial: os.elf
	@echo "Running QEMU with serial..."
	@echo "NOTE the PTY device shown above (e.g., /dev/ttys015)"
	@echo "In another terminal, run: echo 'abc' > /dev/ttys015"
	$(QEMU) $(QFLAGS) -serial pty -monitor none -kernel os.elf

clean:
	rm -f *.elf
