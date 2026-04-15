ccc@teacherdeiMac v0.9.3_struct_packing_fail % ./test.sh
+ gcc -w -g -O1 -fno-omit-frame-pointer c4.c -o c4
+ ./c4 -s test/fib.c
1: #include <stdio.h>
2: 
3: int f(int n) {
4:   if (n<=0) return 0;
    ENT  0
    LLA  2
    LW  
    PSH 
    IMM  0
    LE  
    BZ   0
    IMM  0
    LEV 
5:   if (n==1) return 1;
    LLA  2
    LW  
    PSH 
    IMM  1
    EQ  
    BZ   0
    IMM  1
    LEV 
6:   return f(n-1) + f(n-2);
    LLA  2
    LW  
    PSH 
    IMM  1
    SUB 
    PSH 
    JSR  1
    ADJ  1
    PSH 
    LLA  2
    LW  
    PSH 
    IMM  2
    SUB 
    PSH 
    JSR  1
    ADJ  1
    ADD 
    LEV 
7: }
    LEV 
8: 
9: int main() {
10:   printf("f(7)=%d\n", f(7));
    ENT  0
    IMM  0
    PSH 
    IMM  7
    PSH 
    JSR  1
    ADJ  1
    PSH 
    PRTF
    ADJ  2
11: }
    LEV 
+ ./c4 test/fib.c
f(7)=13
+ ./c4 hello.c
hello, world
+ ./c4 c4.c hello.c
98: Error: bad lvalue in struct
  symtab_base[symtab_count].st_name = name_offset;
                                  ^
+ ./c4 c4.c c4.c hello.c
98: Error: bad lvalue in struct
  symtab_base[symtab_count].st_name = name_offset;
                                  ^
+ ./c4 test/sum_for.c
sum(10)=55
+ ./c4 test/float.c
61: Error: bad lvalue in struct
    p.x = 1.23;
      ^
+ ./c4 test/init.c
Global variables:
global_x = 42
global_pi = 3.141590
msg = Hello from Global String!

Local variables:
a = 30
b = 0
c = 2.500000
+ ./c4 test/error.c
25: Error: bad expression
    b = c + ;
            ^
+ ./c4 test/comment.c
x = 10, y = 20
z = x + y = 30
Multi-line comments work perfectly!
+ ./c4 -o test/fib.elf test/fib.c
>> Successfully written ELF object to test/fib.elf <<
f(7)=13
ccc@teacherdeiMac v0.9.3_struct_packing_fail % ./vm_test.sh
+ gcc -w -g -O1 -fno-omit-frame-pointer vm4.c -o vm4
+ ./c4 -o test/fib.elf test/fib.c
>> Successfully written ELF object to test/fib.elf <<
f(7)=13
+ ./vm4 test/fib.elf
f(7)=13
ccc@teacherdeiMac v0.9.3_struct_packing_fail % ./vm_self.sh
+ gcc -w -g -O1 -fno-omit-frame-pointer c4.c -o c4
+ gcc -w -g -O1 -fno-omit-frame-pointer vm4.c -o vm4
+ ./c4 -o c4.elf c4.c
98: Error: bad lvalue in struct
  symtab_base[symtab_count].st_name = name_offset;
                                  ^
+ ./c4 -o vm4.elf vm4.c
228: Error: bad function call
    memcpy((char *)text_base, file_buf + sec_text->sh_offset, sec_text->sh_size);
                                                                                ^
+ ./c4 -o test/fib.elf test/fib.c
>> Successfully written ELF object to test/fib.elf <<
f(7)=13
+ ./vm4 c4.elf test/fib.c
./vm_self.sh: line 15: 56176 Segmentation fault: 11  ./vm4 c4.elf test/fib.c
+ ./vm4 vm4.elf test/fib.elf
unknown instruction = 4575! cycle = 66360