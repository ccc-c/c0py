set -x
gcc -w -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined c4.c -o c4
#./c4 -s test/fib.c
./c4 test/fib.c
./c4 hello.c
./c4 c4.c hello.c
./c4 c4.c c4.c hello.c
#./c4 -s hello.c
#./c4 c4.c hello.c
#./c4 hello.c
#./c4 c4.c hello.c
#./c4 c4.c c4.c hello.c
./c4 c4.c test/fib.c
./c4 test/sum_for.c
./c4 test/float.c
