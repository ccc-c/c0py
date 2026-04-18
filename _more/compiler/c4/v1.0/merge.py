import re

with open('vm4.c', 'r') as f:
    vm4_content = f.read()

with open('ld4.c', 'r') as f:
    ld4_content = f.read()

elf4_code = """// elf4.c - Standalone ELF Loader, Virtual Machine, and Linker (Merged)
// Self-hostable: written in the subset of C that c4 understands.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>

// -------------------------------------------------------
// Opcodes (must match c4.c exactly)
// -------------------------------------------------------
enum { LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,
       FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF_DBL,
       OPEN,READ,WRIT,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT };

// Rela types (must match c4.c exactly)
enum { RELA_DATA = 1, RELA_FUNC = 2, RELA_JMP = 3 };

// ELF shndx
enum { SHN_UNDEF = 0 };

char *instr_name;
long debug;
long poolsz;

// -------------------------------------------------------
// Helper: little-endian read/write
// -------------------------------------------------------
long r16(char *buf, long p) { return (buf[p] & 0xFF) | ((buf[p+1] & 0xFF) << 8); }
long r32(char *buf, long p) { return r16(buf,p) | (r16(buf,p+2) << 16); }
long r64(char *buf, long p) { return r32(buf,p) | (r32(buf,p+4) << 32); }
void w16(char *buf, long p, long v) { buf[p]=v; buf[p+1]=v>>8; }
void w32(char *buf, long p, long v) { buf[p]=v; buf[p+1]=v>>8; buf[p+2]=v>>16; buf[p+3]=v>>24; }
void w64(char *buf, long p, long v) { w32(buf,p,v); w32(buf,p+4,v>>32); }

// -------------------------------------------------------
// mem_cpy (avoid libc dependency when self-hosted)
// -------------------------------------------------------
void mem_cpy(char *dst, char *src, long n) {
  long i; i=0; while (i<n) { dst[i]=src[i]; i=i+1; }
}

"""

# Extract VM functions
vm_funcs = re.search(r"double long_to_double.*?(?=\n// 記憶體讀取與搬移)", vm4_content, re.DOTALL).group(0)
elf4_code += "// -------------------------------------------------------\n// VM execution functions\n// -------------------------------------------------------\n"
elf4_code += vm_funcs + "\n"

# Extract VM main and rename to load_and_run
vm_main = re.search(r"int main\(int argc, char \*\*argv\) \{(.*?)\n\}", vm4_content, re.DOTALL).group(1)
vm_main = vm_main.replace("int main(int argc, char **argv)", "long load_and_run(char *filename, long vm_argc, char **vm_argv)")
# Modify vm_main body
# 1. remove poolsz init, debug init, instr_name init
# 2. remove arg parsing, use filename directly
load_and_run_code = """long load_and_run(char *filename, long vm_argc, char **vm_argv) {
    long *text_base, *sp, *bp;
    char *data_base;
    long fd;
    
    char *file_buf;
    long file_size;
    long e_shoff, e_shnum;
    long sh_text, sh_data, sh_symtab, sh_strtab;
    long text_off, text_sz, data_off, data_sz;
    long symtab_off, symtab_sz, strtab_off, strtab_sz;
    long i, num_syms, sym_p, name_off;
    char *sym_name;
    long main_offset, text_count, exit_offset;

    fd = open(filename, 0, 0);
    if (fd < 0) { printf("Failed to open %s\\n", filename); return -1; }

    file_buf = (char *)malloc(poolsz);
    file_size = read(fd, file_buf, poolsz);
    close(fd);

    if (file_buf[0] != 0x7f || file_buf[1] != 'E' || file_buf[2] != 'L' || file_buf[3] != 'F') {
        printf("Error: Not a valid ELF file.\\n");
        return -1;
    }

    e_shoff = r64(file_buf, 40);
    e_shnum = r16(file_buf, 60);

    sh_text = e_shoff + 64 * 1;
    sh_data = e_shoff + 64 * 2;
    sh_symtab = e_shoff + 64 * 3;
    sh_strtab = e_shoff + 64 * 4;

    text_off = r64(file_buf, sh_text + 24);
    text_sz  = r64(file_buf, sh_text + 32);

    data_off = r64(file_buf, sh_data + 24);
    data_sz  = r64(file_buf, sh_data + 32);

    symtab_off = r64(file_buf, sh_symtab + 24);
    symtab_sz  = r64(file_buf, sh_symtab + 32);

    strtab_off = r64(file_buf, sh_strtab + 24);
    strtab_sz  = r64(file_buf, sh_strtab + 32);

    text_base = (long *)malloc(text_sz + 8);
    data_base = (char *)malloc(data_sz + 8);
    sp = (long *)malloc(poolsz);

    mem_cpy((char *)text_base, file_buf + text_off, text_sz);
    mem_cpy(data_base, file_buf + data_off, data_sz);

    main_offset = -1;
    num_syms = symtab_sz / 24;
    i = 0;
    while (i < num_syms) {
        sym_p = symtab_off + i * 24;
        name_off = r32(file_buf, sym_p + 0);
        sym_name = file_buf + strtab_off + name_off;

        if (sym_name[0] == 'm' && sym_name[1] == 'a' && sym_name[2] == 'i' && sym_name[3] == 'n' && sym_name[4] == 0) {
            main_offset = r64(file_buf, sym_p + 8);
            break;
        }
        i = i + 1;
    }

    if (main_offset == -1) {
        printf("Error: 'main' symbol not found in ELF!\\n");
        return -1;
    }

    bp = sp = (long *)((long)sp + poolsz);

    text_count = text_sz / sizeof(long);
    exit_offset = text_count - 2;

    *--sp = vm_argc;
    *--sp = (long)vm_argv;
    *--sp = exit_offset;

    return run(text_base + main_offset, bp, sp, data_base, text_base, poolsz);
}
"""
elf4_code += load_and_run_code + "\n"

# Extract ld4 linker parts
linker_vars = re.search(r"char  \*elf_buf\[32\];.*?(?=\n// -------------------------------------------------------\n// Name helpers)", ld4_content, re.DOTALL).group(0)
linker_rest = re.search(r"long name_eq\(char \*a, char \*b\).*?(?=\n// -------------------------------------------------------\n// main)", ld4_content, re.DOTALL).group(0)
# remove redeclaration of poolsz inside linker_rest
linker_rest = linker_rest.replace("long poolsz;\n", "")

elf4_code += "// -------------------------------------------------------\n// Linker State & Functions\n// -------------------------------------------------------\n"
elf4_code += linker_vars + "\n\n"
elf4_code += linker_rest + "\n"

link_elfs_code = """
long link_elfs(char *outfile, long n_elfs) {
  long ei; long i;
  long total_text_longs; long total_data_bytes;
  long *linked_text; char *linked_data;
  long text_longs_i; long data_bytes_i;
  long main_gidx; long entry_offset;
  long exit_slot;

  total_text_longs = 1;
  total_data_bytes = 0;
  ei = 0;
  while (ei < n_elfs) {
    elf_text_base[ei] = total_text_longs;
    elf_data_base[ei] = total_data_bytes;
    total_text_longs = total_text_longs + elf_text_sz[ei] / sizeof(long);
    total_data_bytes = total_data_bytes + elf_data_sz[ei];
    ei = ei + 1;
  }

  total_text_longs = total_text_longs + 2;

  linked_text = (long *)malloc(total_text_longs * sizeof(long));
  linked_data = (char *)malloc(total_data_bytes + 8);
  i = 0; while (i < total_text_longs) { linked_text[i]=0; i=i+1; }
  i = 0; while (i < total_data_bytes + 8) { linked_data[i]=0; i=i+1; }

  ei = 0;
  while (ei < n_elfs) {
    text_longs_i = elf_text_sz[ei] / sizeof(long);
    data_bytes_i = elf_data_sz[ei];
    mem_cpy((char *)(linked_text + elf_text_base[ei]),
            elf_buf[ei] + elf_text_off[ei],
            elf_text_sz[ei]);
    if (data_bytes_i > 0) {
      mem_cpy(linked_data + elf_data_base[ei],
              elf_buf[ei] + elf_data_off[ei],
              data_bytes_i);
    }
    ei = ei + 1;
  }

  if (pass1_collect(n_elfs) < 0) return -1;
  if (pass2_reloc(n_elfs, linked_text, linked_data) < 0) return -1;

  main_gidx = gsym_find("main");
  if (main_gidx < 0 || !gsym_def[main_gidx]) {
    printf("elf4: 'main' not defined\\n"); return -1;
  }
  entry_offset = gsym_val[main_gidx];

  exit_slot = total_text_longs - 2;
  linked_text[exit_slot]     = PSH;
  linked_text[exit_slot + 1] = EXIT;

  i = 1;
  while (i < total_text_longs - 1) {
    if (linked_text[i] == JMP) {
      if (linked_text[i+1] == -1 || linked_text[i+1] == -2) {
        linked_text[i+1] = exit_slot;
      }
      i = i + 2;
    } else {
      i = i + 1;
    }
  }

  write_linked_elf(outfile, linked_text, total_text_longs,
                   linked_data, total_data_bytes, entry_offset);

  printf(">> elf4: linked %ld ELF(s) -> %s (text=%ld longs, data=%ld bytes, entry=%ld) <<\\n",
         n_elfs, outfile, total_text_longs, total_data_bytes, entry_offset);
  return 0;
}
"""
elf4_code += link_elfs_code + "\n"

# Main function unified
main_code = """
// -------------------------------------------------------
// Unified Main
// -------------------------------------------------------
int main(int argc, char **argv) {
  long i, n_elfs, arg_idx;
  char *outfile;
  long is_link;

  poolsz = 1024 * 1024;
  debug = 0;
  outfile = 0;
  n_elfs  = 0;
  gsym_name_ptr = 0;
  gsym_count    = 0;
  gsym_name_pool[0] = 0;
  instr_name = "LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF,OPEN,READ,WRIT,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,";

  is_link = 0;
  i = 1;
  while (i < argc) {
    if (argv[i][0] == '-' && argv[i][1] == 'o' && argv[i][2] == 0) { is_link = 1; break; }
    i = i + 1;
  }

  if (is_link) {
    i = 1;
    while (i < argc) {
      if (argv[i][0] == '-' && argv[i][1] == 'o') {
        i = i + 1;
        if (i >= argc) { printf("elf4: -o requires filename\\n"); return -1; }
        outfile = argv[i];
      } else {
        if (n_elfs >= 32) { printf("elf4: too many input files\\n"); return -1; }
        if (read_elf(n_elfs, argv[i]) < 0) return -1;
        n_elfs = n_elfs + 1;
      }
      i = i + 1;
    }
    if (!outfile) { printf("Usage: elf4 -o out.elf a.elf b.elf ...\\n"); return -1; }
    if (n_elfs == 0) { printf("elf4: no input files\\n"); return -1; }
    return link_elfs(outfile, n_elfs);
  } else {
    arg_idx = 1;
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'd') {
      debug = 1;
      arg_idx = 2;
    }
    if (arg_idx >= argc) {
      printf("Usage: elf4 [-d] <program.elf> [args...]\\n       elf4 -o out.elf a.elf b.elf ...\\n");
      return -1;
    }
    return load_and_run(argv[arg_idx], argc - arg_idx, &argv[arg_idx]);
  }
}
"""
elf4_code += main_code + "\n"

with open('elf4.c', 'w') as f:
    f.write(elf4_code)

print("Merged vm4.c and ld4.c into elf4.c!")
