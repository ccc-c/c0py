// vm4.c - Standalone ELF Loader and Virtual Machine (Self-Hostable)
// Now using elegant standard C Structs with full packing support!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>

enum { LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,LW  ,LH  ,SI  ,SC  ,SW  ,SH  ,PSH ,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,
       FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF_DBL,
       OPEN,READ,WRIT,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT };

char *instr_name = "LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,LW  ,LH  ,SI  ,SC  ,SW  ,SH  ,PSH ,OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF,OPEN,READ,WRIT,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,";
long debug = 0;

// 現在我們可以直接使用完美對齊的結構！
struct Elf64_Ehdr {
    char e_ident[16];
    short e_type;      short e_machine;
    int e_version;     long e_entry;
    long e_phoff;      long e_shoff;
    int e_flags;       short e_ehsize;
    short e_phentsize; short e_phnum;
    short e_shentsize; short e_shnum;
    short e_shstrndx;
};

struct Elf64_Shdr {
    int sh_name;       int sh_type;
    long sh_flags;      long sh_addr;
    long sh_offset;     long sh_size;
    int sh_link;       int sh_info;
    long sh_addralign;  long sh_entsize;
};

struct Elf64_Sym {
    int st_name;    char st_info;
    char st_other;  short st_shndx;
    long st_value;  long st_size;
};

double long_to_double(long v) { return *(double *)&v; }
long   double_to_long(double d) { return *(long *)&d; }

long to_addr(long addr, long d_base, long poolsz) {
    if (addr >= 0) {
        if (addr < poolsz) return d_base + addr;
    }
    return addr;
}

long run(long *pc, long *bp, long *sp, char *d_base, long *t_base, long poolsz) {
  long a, cycle, i, *t, t_addr, ai, cv;
  double fa, fb, dv;
  char *fmt, *f2, *sp3, spec[32];
  long fargs[5];

  cycle = 0;
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
    
    // 全新的 LH, LW 等 2/4-byte 載入與儲存指令支援
    else if (i == LI)  a = *(long *)to_addr(a, (long)d_base, poolsz);
    else if (i == LC)  a = *(char *)to_addr(a, (long)d_base, poolsz);
    else if (i == LW)  a = *(int *)to_addr(a, (long)d_base, poolsz);
    else if (i == LH)  a = *(short *)to_addr(a, (long)d_base, poolsz);
    else if (i == SI)  { t_addr = *sp++; *(long *)to_addr(t_addr, (long)d_base, poolsz) = a; }
    else if (i == SC)  { t_addr = *sp++; a = *(char *)to_addr(t_addr, (long)d_base, poolsz) = a; }
    else if (i == SW)  { t_addr = *sp++; *(int *)to_addr(t_addr, (long)d_base, poolsz) = a; }
    else if (i == SH)  { t_addr = *sp++; *(short *)to_addr(t_addr, (long)d_base, poolsz) = a; }
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

int main(int argc, char **argv) {
    long poolsz;
    long *text_base, *sp, *bp;
    char *data_base;
    long fd;
    char *filename;
    long arg_idx;
    
    char *file_buf;
    long file_size;
    struct Elf64_Ehdr *ehdr;
    struct Elf64_Shdr *shdrs;
    struct Elf64_Shdr *sec_text, *sec_data, *sec_symtab, *sec_strtab;
    struct Elf64_Sym *syms, *sym;
    char *strs;
    
    long main_offset, text_count, exit_offset, num_syms;
    long i;
    char *sym_name;

    poolsz = 1024 * 1024;
    arg_idx = 1;

    if (argc > 1) {
        if (argv[1][0] == '-' && argv[1][1] == 'd') {
            debug = 1;
            arg_idx = arg_idx + 1;
        }
    }
    if (arg_idx >= argc) {
        printf("Usage: vm4 [-d] <program.elf> [args...]\n");
        return -1;
    }
    filename = argv[arg_idx];

    fd = open(filename, 0, 0);
    if (fd < 0) { printf("Failed to open %s\n", filename); return -1; }

    file_buf = (char *)malloc(poolsz);
    file_size = read(fd, file_buf, poolsz);
    close(fd);

    ehdr = (struct Elf64_Ehdr *)file_buf;
    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        printf("Error: Not a valid ELF file.\n");
        return -1;
    }

    // 透過 char* 位移來保證精準抓取 Struct 陣列元素
    shdrs = (struct Elf64_Shdr *)(file_buf + ehdr->e_shoff);
    sec_text   = (struct Elf64_Shdr *)((char *)shdrs + 1 * sizeof(struct Elf64_Shdr));
    sec_data   = (struct Elf64_Shdr *)((char *)shdrs + 2 * sizeof(struct Elf64_Shdr));
    sec_symtab = (struct Elf64_Shdr *)((char *)shdrs + 3 * sizeof(struct Elf64_Shdr));
    sec_strtab = (struct Elf64_Shdr *)((char *)shdrs + 4 * sizeof(struct Elf64_Shdr));

    text_base = (long *)malloc(sec_text->sh_size + 8);
    data_base = (char *)malloc(sec_data->sh_size + 8);
    sp = (long *)malloc(poolsz);

    memcpy((char *)text_base, file_buf + sec_text->sh_offset, sec_text->sh_size);
    memcpy(data_base, file_buf + sec_data->sh_offset, sec_data->sh_size);

    syms = (struct Elf64_Sym *)(file_buf + sec_symtab->sh_offset);
    strs = file_buf + sec_strtab->sh_offset;

    main_offset = -1;
    num_syms = sec_symtab->sh_size / sizeof(struct Elf64_Sym);
    
    i = 0;
    while (i < num_syms) {
        sym = (struct Elf64_Sym *)((char *)syms + i * sizeof(struct Elf64_Sym));
        sym_name = strs + sym->st_name;
        if (sym_name[0] == 'm' && sym_name[1] == 'a' && sym_name[2] == 'i' && sym_name[3] == 'n' && sym_name[4] == 0) {
            main_offset = sym->st_value;
            break;
        }
        i = i + 1;
    }

    if (main_offset == -1) {
        printf("Error: 'main' symbol not found in ELF!\n");
        return -1;
    }

    bp = sp = (long *)((long)sp + poolsz);
    text_count = sec_text->sh_size / sizeof(long);
    exit_offset = text_count - 2;

    *--sp = argc - arg_idx;
    *--sp = (long)&argv[arg_idx];
    *--sp = exit_offset;

    return run(text_base + main_offset, bp, sp, data_base, text_base, poolsz);
}