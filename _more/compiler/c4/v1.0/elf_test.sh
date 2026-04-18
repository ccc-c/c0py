set -x
# 1. 以 GCC 編譯基礎編譯器與虛擬機器
gcc -w -g -O1 -fno-omit-frame-pointer c4.c -o c4
gcc -w -g -O1 -fno-omit-frame-pointer elf4.c -o elf4

# 2. 測試基本 ELF 的編譯與載入執行
./c4 -o test/fib.elf test/fib.c
./elf4 test/fib.elf

# 3. 測試多重 ELF 檔案的靜態連結器 (Linker) 功能
./c4 -o test/lib_math.elf test/lib_math.c
./c4 -o test/main_ext.elf test/main_ext.c
./elf4 -o test/run.elf test/lib_math.elf test/main_ext.elf
./elf4 test/run.elf

# 4. 測試 Self-hosting 虛擬機自我編譯
./c4 -o c4.elf c4.c
./c4 -o elf4.elf elf4.c

# 5. 確保在虛擬機 (elf4) 上的 c4.elf 可以再編譯與被載入執行
./elf4 c4.elf -o test/fib2.elf test/fib.c
./elf4 elf4.elf test/fib2.elf
