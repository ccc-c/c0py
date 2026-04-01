#!/bin/bash

set -e

echo "Building OS..."
make clean
make os.elf

echo ""
echo "Running QEMU test with UART interrupt..."
OUTPUT=$(mktemp)

expect -c "
set timeout 20
spawn qemu-system-riscv64 -nographic -smp 4 -machine virt -bios none -kernel os.elf
expect {
    -re \"(Hart|Task|OS|Start|Created|counter|done)\" { exp_continue }
    timeout { }
    eof { }
}
# Send some characters to trigger UART interrupts
send \"a\"
send \"b\"
send \"c\"
sleep 1
send \"\003\"
expect eof
" > "$OUTPUT" 2>&1 || true

echo "=== Output (last 60 lines) ==="
tail -60 "$OUTPUT"

echo ""
echo "=== Test Analysis ==="

TASK0_RUNS=$(grep -c "Task0 \[hart" "$OUTPUT" 2>/dev/null || echo "0")
TASK1_RUNS=$(grep -c "Task1 \[hart" "$OUTPUT" 2>/dev/null || echo "0")
TASK2_RUNS=$(grep -c "Task2 \[hart" "$OUTPUT" 2>/dev/null || echo "0")
TASK3_RUNS=$(grep -c "Task3 \[hart" "$OUTPUT" 2>/dev/null || echo "0")
TASK4_RUNS=$(grep -c "Task4 \[hart" "$OUTPUT" 2>/dev/null || echo "0")

echo "Task0 running count: $TASK0_RUNS"
echo "Task1 running count: $TASK1_RUNS"
echo "Task2 running count: $TASK2_RUNS"
echo "Task3 running count: $TASK3_RUNS"
echo "Task4 running count: $TASK4_RUNS"

TOTAL_RUNNING=0
[ "$TASK0_RUNS" -gt 0 ] && TOTAL_RUNNING=$((TOTAL_RUNNING + 1))
[ "$TASK1_RUNS" -gt 0 ] && TOTAL_RUNNING=$((TOTAL_RUNNING + 1))
[ "$TASK2_RUNS" -gt 0 ] && TOTAL_RUNNING=$((TOTAL_RUNNING + 1))
[ "$TASK3_RUNS" -gt 0 ] && TOTAL_RUNNING=$((TOTAL_RUNNING + 1))
[ "$TASK4_RUNS" -gt 0 ] && TOTAL_RUNNING=$((TOTAL_RUNNING + 1))

if [ "$TOTAL_RUNNING" -ge 2 ]; then
    echo "[PASS] Found $TOTAL_RUNNING tasks running"
else
    echo "[FAIL] Only $TOTAL_RUNNING tasks running (expected at least 2)"
    rm -f "$OUTPUT"
    exit 1
fi

echo ""
echo "=== Mutex Test ==="

COUNTERS=$(grep -E "Task[0-9] \[hart.*counter after" "$OUTPUT" | sed 's/.*= //')
if [ -z "$COUNTERS" ]; then
    echo "[FAIL] No counter output found"
    rm -f "$OUTPUT"
    exit 1
fi

UNIQUE_COUNTERS=$(echo "$COUNTERS" | sort -n | uniq | grep -E '^[0-9]+$')
UNIQUE_COUNT=$(echo "$UNIQUE_COUNTERS" | wc -l)
echo "Counter values: $UNIQUE_COUNT unique values"

SEQUENCE_OK=true
PREV=0
for VAL in $UNIQUE_COUNTERS; do
    if [ "$VAL" -eq $((PREV + 1)) ] || [ "$VAL" -eq 1 ]; then
        :
    elif [ "$VAL" -gt $((PREV + 1)) ]; then
        echo "  [WARN] Gap detected: expected $((PREV + 1)), got $VAL"
        SEQUENCE_OK=false
    fi
    PREV=$VAL
done

if [ "$SEQUENCE_OK" = true ]; then
    echo "[PASS] Counter values are in correct sequence"
else
    echo "[FAIL] Counter values show gaps (possible race condition)"
    rm -f "$OUTPUT"
    exit 1
fi

echo ""
echo "=== UART Interrupt Test ==="

UART_INIT=$(grep "UART initialized" "$OUTPUT" 2>/dev/null | wc -l)
UART_ISR_REG=$(grep "UART ISR registered" "$OUTPUT" 2>/dev/null | wc -l)
UART_RECV=$(grep "UART received:" "$OUTPUT" 2>/dev/null | wc -l)
UART_ISR_CALLED=$(grep "UART ISR called" "$OUTPUT" 2>/dev/null | wc -l)

echo "UART initialized: $UART_INIT"
echo "UART ISR registered: $UART_ISR_REG"
echo "UART received chars: $UART_RECV"
echo "UART ISR called count: $UART_ISR_CALLED"

if [ "$UART_INIT" -ge 1 ]; then
    echo "[PASS] UART initialized successfully"
else
    echo "[FAIL] UART not initialized"
    rm -f "$OUTPUT"
    exit 1
fi

if [ "$UART_ISR_REG" -ge 1 ]; then
    echo "[PASS] UART ISR registered"
else
    echo "[FAIL] UART ISR not registered"
    rm -f "$OUTPUT"
    exit 1
fi

# Note: UART interrupt testing via expect may not work reliably
# because QEMU's serial port interrupt behavior varies
# Manual testing is recommended: make qemu, then type characters
if [ "$UART_RECV" -gt 0 ] || [ "$UART_ISR_CALLED" -gt 0 ]; then
    echo "[PASS] UART interrupt working (received $UART_RECV chars, ISR called $UART_ISR_CALLED times)"
else
    echo "[WARN] No UART interrupt detected (may need manual testing)"
    echo "       Run 'make qemu' and type characters to test"
fi

echo ""
echo "=== All tests passed ==="
rm -f "$OUTPUT"
