 gcc -w -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined ld4.c -o ld4
./c4 -o test/lib_math.elf  test/lib_math.c          # 含 extern 定義的函式庫
./c4 -o test/main_ext.elf test/main_ext.c         # 含 extern 宣告的主程式
./ld4 -o test/run.elf test/lib_math.elf test/main_ext.elf
./vm4 test/run.elf
