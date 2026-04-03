#include <stdio.h>
#include <stdlib.h>

int factorial_while(int n) {
    int result;
    result = 1;
    while (n > 1) {
    result = (result * n);
    n = (n - 1);
    }
    return result;
}

int main() {
    int result = factorial_while(5);
    printf("factorial_while(5)=%d\n", result);
    return 0;
}