// vm4.c - A Standalone ELF Loader and Virtual Machine for c4
// Usage: ./vm4 [-d] <program.elf> [args...]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

// -------------------------------------------------------
// 1. VM Opcodes (必須與 c4.c 完全一致)
// -------------------------------------------------------
enum { LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,
       FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF_DBL,
       OPEN,READ,WRIT,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT };

char *instr_name = "LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF,OPEN,READ,WRIT,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,";
long debug = 0;

// -------------------------------------------------------
// 2. ELF64 資料結構 (標準規格)
// -------------------------------------------------------
struct Elf64_Ehdr {
    unsigned char e_ident[16];
    uint16_t e_type;      uint16_t e_machine;
    uint32_t e_version;   uint64_t e_entry;
    uint64_t e_phoff;     uint64_t e_shoff;
    uint32_t e_flags;     uint16_t e_ehsize;
    uint16_t e_phentsize; uint16_t e_phnum;
    uint16_t e_shentsize; uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf64_Shdr {
    uint32_t sh_name;       uint32_t sh_type;
    uint64_t sh_flags;      uint64_t sh_addr;
    uint64_t sh_offset;     uint64_t sh_size;
    uint32_t sh_link;       uint32_t sh_info;
    uint64_t sh_addralign;  uint64_t sh_entsize;
};

struct Elf64_Sym {
    uint32_t st_name;   uint8_t  st_info;
    uint8_t  st_other;  uint16_t st_shndx;
    uint64_t st_value;  uint64_t st_size;
};

// -------------------------------------------------------
// 3. VM 輔助函數與核心執行器 (完全照搬 c4.c)
// -------------------------------------------------------
#define LAST_WITH_OPERAND ADJ
double long_to_double(long v) { return *(double *)&v; }
long   double_to_long(double d) { return *(long *)&d; }

long to_addr(long addr, long d_base, long poolsz) {
    if (addr >= 0 && addr < poolsz) return d_base + addr;
    return addr;
}

long run(long *pc, long *bp, long *sp, char *d_base, long *t_base, long poolsz) {
  long a, cycle = 0, i, *t, t_addr, ai, cv;
  double fa, fb, dv;
  char *fmt, *f2, *sp3, spec[32];
  long fargs[5];

  while (1) {
    i = *pc++; ++cycle;
    if (debug) {
      printf("%ld> %.4s", cycle, instr_name + i * 5);
      if (i <= ADJ || i == IMMF) printf(" %ld\n", *pc); else printf("\n");
    }

    if      (i == LLA) a = (long)(bp + *pc++);
    else if (i == IMM) a = *pc++;
    else if (i == JMP) pc = t_base + *pc;
    else if (i == JSR) { *--sp = (long)(pc + 1 - t_base); pc = t_base + *pc; } 
    else if (i == BZ)  pc = a ? pc + 1 : t_base + *pc;
    else if (i == BNZ) pc = a ? t_base + *pc : pc + 1;
    else if (i == ENT) { *--sp = (long)bp; bp = sp; sp = sp - *pc++; }
    else if (i == ADJ) sp = sp + *pc++;
    else if (i == LEV) { sp = bp; bp = (long *)*sp++; pc = t_base + *sp++; }
    
    else if (i == LI)  a = *(long *)to_addr(a, (long)d_base, poolsz);
    else if (i == LC)  a = *(char *)to_addr(a, (long)d_base, poolsz);
    else if (i == SI)  { t_addr = *sp++; *(long *)to_addr(t_addr, (long)d_base, poolsz) = a; }
    else if (i == SC)  { t_addr = *sp++; a = *(char *)to_addr(t_addr, (long)d_base, poolsz) = a; }
    else if (i == PSH) *--sp = a;

    else if (i == OR)  a = *sp++ |  a;
    else if (i == XOR) a = *sp++ ^  a;
    else if (i == AND) a = *sp++ &  a;
    else if (i == EQ)  a = *sp++ == a;
    else if (i == NE)  a = *sp++ != a;
    else if (i == LT)  a = *sp++ <  a;
    else if (i == GT)  a = *sp++ >  a;
    else if (i == LE)  a = *sp++ <= a;
    else if (i == GE)  a = *sp++ >= a;
    else if (i == SHL) a = *sp++ << a;
    else if (i == SHR) a = *sp++ >> a;
    else if (i == ADD) a = *sp++ +  a;
    else if (i == SUB) a = *sp++ -  a;
    else if (i == MUL) a = *sp++ * a;
    else if (i == DIV) a = *sp++ /  a;
    else if (i == MOD) a = *sp++ %  a;

    else if (i == IMMF) { a = *pc++; } 
    else if (i == LF) { a = double_to_long(*(double *)to_addr(a, (long)d_base, poolsz)); }
    else if (i == SF) { t_addr = *sp++; *(double *)to_addr(t_addr, (long)d_base, poolsz) = long_to_double(a); }
    else if (i == ITF) { fa = (double)a; a = double_to_long(fa); }
    else if (i == ITFS) { fa = (double)(*sp); *sp = double_to_long(fa); }
    else if (i == FTI) { fa = long_to_double(a); a = (long)fa; }

    else if (i == FADD) { fa = long_to_double(*sp++) + long_to_double(a); a = double_to_long(fa); }
    else if (i == FSUB) { fa = long_to_double(*sp++) - long_to_double(a); a = double_to_long(fa); }
    else if (i == FMUL) { fa = long_to_double(*sp++) * long_to_double(a); a = double_to_long(fa); }
    else if (i == FDIV) { fa = long_to_double(*sp++) / long_to_double(a); a = double_to_long(fa); }

    else if (i == FEQ) { fa = long_to_double(*sp++); fb = long_to_double(a); a = (fa == fb); }
    else if (i == FNE) { fa = long_to_double(*sp++); fb = long_to_double(a); a = (fa != fb); }
    else if (i == FLT) { fa = long_to_double(*sp++); fb = long_to_double(a); a = (fa <  fb); }
    else if (i == FGT) { fa = long_to_double(*sp++); fb = long_to_double(a); a = (fa >  fb); }
    else if (i == FLE) { fa = long_to_double(*sp++); fb = long_to_double(a); a = (fa <= fb); }
    else if (i == FGE) { fa = long_to_double(*sp++); fb = long_to_double(a); a = (fa >= fb); }

    // [修改] 支援 3 參數的 open，並加入 write
    else if (i == OPEN) a = open((char *)to_addr(sp[2], (long)d_base, poolsz), sp[1], *sp);
    else if (i == READ) a = read(sp[2], (char *)to_addr(sp[1], (long)d_base, poolsz), *sp);
    else if (i == WRIT) a = write(sp[2], (char *)to_addr(sp[1], (long)d_base, poolsz), *sp);
    else if (i == CLOS) a = close(*sp);
    else if (i == PRTF) {
      t = sp + pc[1];
      fmt = (char *)to_addr(t[-1], (long)d_base, poolsz);
      fargs[0]=t[-2]; fargs[1]=t[-3]; fargs[2]=t[-4]; fargs[3]=t[-5]; fargs[4]=t[-6];
      f2 = fmt; ai = 0; a = 0;
      while (*f2) {
        if (*f2 != '%') { printf("%c", *f2++); a = a + 1; continue; }
        sp3 = spec; *sp3++ = *f2++;
        while (*f2=='-'||*f2=='+'||*f2==' '||*f2=='#'||*f2=='0') *sp3++ = *f2++;
        while (*f2>='0' && *f2<='9') *sp3++ = *f2++;
        if (*f2 == '.') { *sp3++ = *f2++; while (*f2>='0'&&*f2<='9') *sp3++ = *f2++; }
        if (*f2=='l'||*f2=='h'||*f2=='L') *sp3++ = *f2++;
        if (*f2=='l') *sp3++ = *f2++;
        cv = *f2 ? *f2++ : 0;
        *sp3++ = cv; *sp3 = 0;
        if (cv=='f'||cv=='g'||cv=='e'||cv=='F'||cv=='G'||cv=='E') {
          dv = long_to_double(fargs[ai<5?ai:4]);
          a = a + printf(spec, dv);
        } else if (cv=='%') {
          a = a + printf("%%");
        } else if (cv=='s') {
          a = a + printf(spec, (char *)to_addr(fargs[ai<5?ai:4], (long)d_base, poolsz));
        } else {
          a = a + printf(spec, fargs[ai<5?ai:4]);
        }
        if (cv != '%') ai = ai + 1;
      }
    }
    else if (i == MALC) a = (long)malloc(*sp);
    else if (i == FREE) free((void *)*sp);
    else if (i == MSET) a = (long)memset((char *)to_addr(sp[2], (long)d_base, poolsz), sp[1], *sp);
    else if (i == MCMP) a = memcmp((char *)to_addr(sp[2], (long)d_base, poolsz), (char *)to_addr(sp[1], (long)d_base, poolsz), *sp);
    else if (i == EXIT) { if (debug) printf("exit(%ld) cycle = %ld\n", *sp, cycle); return *sp; }
    
    else { printf("unknown instruction = %ld! cycle = %ld\n", i, cycle); return -1; }
  }
}

// -------------------------------------------------------
// 4. ELF 載入器主程式
// -------------------------------------------------------
int main(int argc, char **argv) {
    long poolsz = 1024 * 1024;
    long *text_base, *sp, *bp;
    char *data_base;
    FILE *f;
    char *filename = NULL;
    
    // 解析參數
    int arg_idx = 1;
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debug = 1;
        arg_idx++;
    }
    if (arg_idx >= argc) {
        printf("Usage: vm4 [-d] <program.elf> [args...]\n");
        return -1;
    }
    filename = argv[arg_idx];

    // 打開 ELF 檔案
    f = fopen(filename, "rb");
    if (!f) { printf("Failed to open %s\n", filename); return -1; }

    // 讀取 ELF Header
    struct Elf64_Ehdr ehdr;
    fread(&ehdr, 1, sizeof(ehdr), f);
    if (memcmp(ehdr.e_ident, "\x7f" "ELF", 4) != 0) {
        printf("Error: Not a valid ELF file.\n");
        return -1;
    }

    // 讀取 Section Headers
    struct Elf64_Shdr *shdrs = malloc(sizeof(struct Elf64_Shdr) * ehdr.e_shnum);
    fseek(f, ehdr.e_shoff, SEEK_SET);
    fread(shdrs, sizeof(struct Elf64_Shdr), ehdr.e_shnum, f);

    // 根據 c4.c 的寫入邏輯，Section 的順序是固定的：
    // [1]: .text, [2]: .data, [3]: .symtab, [4]: .strtab
    struct Elf64_Shdr *sec_text = &shdrs[1];
    struct Elf64_Shdr *sec_data = &shdrs[2];
    struct Elf64_Shdr *sec_symtab = &shdrs[3];
    struct Elf64_Shdr *sec_strtab = &shdrs[4];

    // 配置記憶體池
    text_base = (long *)calloc(1, poolsz);
    data_base = (char *)calloc(1, poolsz);
    sp = (long *)calloc(1, poolsz);

    // 載入 .text 區段
    fseek(f, sec_text->sh_offset, SEEK_SET);
    fread(text_base, 1, sec_text->sh_size, f);

    // 載入 .data 區段
    fseek(f, sec_data->sh_offset, SEEK_SET);
    fread(data_base, 1, sec_data->sh_size, f);

    // 載入 .symtab 區段
    struct Elf64_Sym *syms = malloc(sec_symtab->sh_size);
    fseek(f, sec_symtab->sh_offset, SEEK_SET);
    fread(syms, 1, sec_symtab->sh_size, f);

    // 載入 .strtab 區段
    char *strs = malloc(sec_strtab->sh_size);
    fseek(f, sec_strtab->sh_offset, SEEK_SET);
    fread(strs, 1, sec_strtab->sh_size, f);

    fclose(f);

    // 尋找 main 函數的 Offset
    long main_offset = -1;
    int num_syms = sec_symtab->sh_size / sizeof(struct Elf64_Sym);
    for (int i = 0; i < num_syms; i++) {
        char *sym_name = strs + syms[i].st_name;
        if (strcmp(sym_name, "main") == 0) {
            main_offset = syms[i].st_value;
            break;
        }
    }

    if (main_offset == -1) {
        printf("Error: 'main' symbol not found in ELF!\n");
        return -1;
    }

    // 準備執行環境 (Stack)
    bp = sp = (long *)((long)sp + poolsz);

    // 取得離開指令的 Offset
    long text_count = sec_text->sh_size / sizeof(long);
    long exit_offset = text_count - 2;

    // 將執行參數推入堆疊 (從 filename 開始算 argv)
    *--sp = argc - arg_idx;     // argc
    *--sp = (long)&argv[arg_idx]; // argv
    *--sp = exit_offset;        // return address

    // 啟動虛擬機
    return run(text_base + main_offset, bp, sp, data_base, text_base, poolsz);
}