#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>

int main() {
    printf("Testing JIT code generation...\n");
    
    uint8_t *code = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
    
    printf("mmap: %p\n", code);
    
    if (code == MAP_FAILED || code == NULL) {
        perror("mmap");
        return 1;
    }
    
    uint8_t *p = code;
    
    // movz x0, #1
    *p++ = 0x20; *p++ = 0x00; *p++ = 0x80; *p++ = 0xd2;
    // ret
    *p++ = 0xc0; *p++ = 0x03; *p++ = 0x5f; *p++ = 0xd6;
    
    printf("code written\n");
    
    int r = mprotect(code, 4096, PROT_READ | PROT_EXEC);
    printf("mprotect: %d\n", r);
    if (r != 0) {
        perror("mprotect");
    }
    
    printf("calling code...\n");
    
    typedef long (*Func)();
    Func f = (Func)code;
    long result = f();
    
    printf("Result: %ld (expected 1)\n", result);
    
    return 0;
}
