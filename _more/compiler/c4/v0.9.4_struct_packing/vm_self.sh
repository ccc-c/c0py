set -x
# gcc -w -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined c4.c -o c4
gcc -w -g -O1 -fno-omit-frame-pointer c4.c -o c4
# gcc -w -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined vm4.c -o vm4
gcc -w -g -O1 -fno-omit-frame-pointer vm4.c -o vm4

./c4 -o c4.elf c4.c
./c4 -o vm4.elf vm4.c

./c4 -o test/fib.elf test/fib.c


# ./vm4 c4.elf test/fib.elf
# 讓被虛擬的 c4 去編譯 fib.c
./vm4 c4.elf test/fib.c
./vm4 vm4.elf test/fib.elf