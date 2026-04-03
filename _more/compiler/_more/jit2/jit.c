#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>
#include <dlfcn.h>

typedef enum {
    OP_LOADI,
    OP_ADD,
    OP_MUL,
    OP_RET
} OpCode;

typedef struct {
    OpCode op;
    int a;
    int b;
} Instruction;

Instruction program[] = {
    {OP_LOADI, 1, 0},
    {OP_LOADI, 2, 0},
    {OP_MUL,  0, 0},
    {OP_LOADI, 3, 0},
    {OP_ADD,  0, 0},
    {OP_RET,  0, 0},
};

int interpreter() {
    int pc = 0;
    int a = 0, b = 0;
    
    printf("  Running interpreter...\n");
    while (1) {
        Instruction *instr = &program[pc++];
        printf("    PC=%d: ", pc-1);
        switch (instr->op) {
            case OP_LOADI:
                printf("LOADI %d\n", instr->a);
                if (a == 0) a = instr->a;
                else b = instr->a;
                break;
            case OP_ADD:
                printf("ADD %d + %d\n", a, b);
                a = a + b;
                b = 0;
                break;
            case OP_MUL:
                printf("MUL %d * %d\n", a, b);
                a = a * b;
                b = 0;
                break;
            case OP_RET:
                printf("RET => %d\n", a);
                return a;
        }
    }
}

typedef int (*JitFunc)();

JitFunc jit_compile(Instruction *program, int len) {
    FILE *f = fopen("jit_func.c", "w");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "int jit_func() {\n");
    fprintf(f, "    int a = 0, b = 0;\n");
    
    for (int i = 0; i < len; i++) {
        fprintf(f, "    /* PC=%d */\n", i);
        switch (program[i].op) {
            case OP_LOADI:
                if (i == 0) 
                    fprintf(f, "    a = %d;\n", program[i].a);
                else
                    fprintf(f, "    b = %d;\n", program[i].a);
                break;
            case OP_ADD:
                fprintf(f, "    a = a + b; b = 0;\n");
                break;
            case OP_MUL:
                fprintf(f, "    a = a * b; b = 0;\n");
                break;
            case OP_RET:
                fprintf(f, "    return a;\n");
                break;
        }
    }
    fprintf(f, "}\n");
    fclose(f);
    
    printf("  Generated C code: jit_func.c\n");
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "clang -shared -o jit_func.so jit_func.c -fPIC 2>/dev/null");
    system(cmd);
    printf("  Compiled to shared library: jit_func.so\n");
    
    void *handle = dlopen("./jit_func.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        exit(1);
    }
    
    JitFunc func = (JitFunc)dlsym(handle, "jit_func");
    return func;
}

int main() {
    printf("========================================\n");
    printf("   JIT (Just-In-Time) Compiler Demo\n");
    printf("   Target: Apple M3 (ARM64)\n");
    printf("========================================\n\n");
    printf("Program: 1 * 2 + 3\n");
    printf("Bytecode:\n");
    for (int i = 0; i < 6; i++) {
        printf("  [%d] ", i);
        switch (program[i].op) {
            case OP_LOADI: printf("LOADI %d\n", program[i].a); break;
            case OP_ADD:   printf("ADD\n"); break;
            case OP_MUL:   printf("MUL\n"); break;
            case OP_RET:   printf("RET\n"); break;
        }
    }
    printf("\n");

    printf("--- Interpreter ---\n");
    int result1 = interpreter();
    printf("Result: %d\n\n", result1);

    printf("--- JIT Compiler ---\n");
    JitFunc jit = jit_compile(program, 6);
    int result2 = jit();
    printf("Result: %d\n\n", result2);

    printf("========================================\n");
    printf("Interpretation: 每條指令都要透過 switch-case\n");
    printf("JIT: 在執行時即時編譯成機器碼並執行\n");
    printf("========================================\n");
    return 0;
}
