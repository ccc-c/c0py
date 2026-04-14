set -x
gcc -w -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined vm4.c -o vm4
./c4 -o test/fib.elf test/fib.c
./vm4 test/fib.elf