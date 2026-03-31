.equ STACK_SIZE, 8192

.global _start

_start:
    # Debug: write '1\n' to UART
    lui t0, 0x10000
    li t1, 0x31  # '1'
    sb t1, 0(t0)
    li t1, 0x0a  # '\n'
    sb t1, 0(t0)
    
    # set M Previous Privilege mode to Supervisor, for mret.
    csrr t0, mstatus
    li t1, -1
    and t0, t0, t1
    ori t0, t0, 0x200
    csrw mstatus, t0

    # Debug: write '2\n'
    li t1, 0x32
    sb t1, 0(t0)
    li t1, 0x0a
    sb t1, 0(t0)

    # set M Exception Program Counter to os_main
    la t0, os_main
    csrw mepc, t0

    # Debug: write '3\n'
    li t1, 0x33
    sb t1, 0(t0)
    li t1, 0x0a
    sb t1, 0(t0)

    # disable paging
    csrw satp, zero

    # delegate all interrupts to supervisor mode
    csrw medeleg, zero
    csrw mideleg, zero

    # Debug: write '4\n'
    li t1, 0x34
    sb t1, 0(t0)
    li t1, 0x0a
    sb t1, 0(t0)

    # enable supervisor timer interrupt
    csrr t0, sie
    ori t0, t0, 0x22
    csrw sie, t0

    # configure PMP
    lui t0, 0xfffff
    addi t0, t0, -1
    csrw pmpaddr0, t0
    csrw pmpcfg0, 0xf

    # Debug: write '5\n'
    li t1, 0x35
    sb t1, 0(t0)
    li t1, 0x0a
    sb t1, 0(t0)

    # set up stack - same as xv6 entry.S
    la sp, stacks
    li t0, 4096
    csrr t1, mhartid
    addi t1, t1, 1
    mul t0, t0, t1
    add sp, sp, t0

    # Debug: write '6\n'
    li t1, 0x36
    sb t1, 0(t0)
    li t1, 0x0a
    sb t1, 0(t0)

    # mret to supervisor mode -> os_main
    mret

_end:
    j _end

stacks:
    .skip 4096 * 4
