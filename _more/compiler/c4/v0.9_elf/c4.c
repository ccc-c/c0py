// c4_elf_phase2.c - C in four functions (ELF Phase 2: ELF File Generator)
//
// Modified to support generating real ELF64 Relocatable/Executable files.
// Added -o xxx.elf argument. Fully self-compilable.

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <math.h>

struct symbol {
  long v_tk;
  long v_hash;
  char *v_name;
  long v_class;
  long v_type;
  long v_val;
  long v_hclass;
  long v_htype;
  long v_hval;
};

// -------------------------------------------------------
// ELF 符號表結構
// -------------------------------------------------------
struct Elf64_Sym {
  long st_name;
  long st_info;
  long st_other;
  long st_shndx;
  long st_value;
  long st_size;
};

char *data_base;
long *text_base;
struct Elf64_Sym *symtab_base;
char *strtab_base;
long symtab_count;
long strtab_ptr;

char *p, *lp,
     *data;

long *e, *le,
    tk,
    ival,
    ty,
    loc,
    line,
    src,
    debug;

double fval;
struct symbol *sym, *id;

enum {
  Num = 128, NumF, Fun, Sys, Glo, GloArr, Loc, LocArr, Id, Member,
  Char, Double, Else, Enum, Float, For, Break, Continue, If, Int, Int64, Long, Return, Short, Sizeof, While, Struct,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak, Arrow, Dot
};

enum { LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,
       FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF_DBL,
       OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,FOPN,FWRT,FCLS };

enum { CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, PTR };
enum { Idsz = 9 };

double long_to_double(long v) { return *(double *)&v; }
long   double_to_long(double d) { return *(long *)&d; }

char *instr_name;
#define LAST_WITH_OPERAND ADJ

int is_float_ty(long t) { return (t == FLOAT || t == DOUBLE); }

void error(char *msg) {
  char *ep;
  printf("%ld: Error: %s\n", line, msg);
  ep = lp;
  while (*ep && *ep != '\n') { printf("%c", *ep); ep = ep + 1; }
  printf("\n");
  ep = lp;
  while (ep < p - 1) {
    if (*ep == '\t') printf("\t");
    else printf(" ");
    ep = ep + 1;
  }
  printf("^\n");
  exit(-1);
}

// -------------------------------------------------------
// 註冊符號到 .symtab 與 .strtab
// -------------------------------------------------------
long add_symbol(struct symbol *s, long type, long value) {
  long len;
  long name_offset;
  long i;
  
  len = s->v_hash & 0x3F;
  name_offset = strtab_ptr;
  
  i = 0;
  while (i < len) {
    strtab_base[strtab_ptr] = s->v_name[i];
    strtab_ptr = strtab_ptr + 1;
    i = i + 1;
  }
  strtab_base[strtab_ptr] = 0;
  strtab_ptr = strtab_ptr + 1;

  symtab_base[symtab_count].st_name = name_offset;
  symtab_base[symtab_count].st_info = type;
  symtab_base[symtab_count].st_other = 0;
  symtab_base[symtab_count].st_shndx = 1;
  symtab_base[symtab_count].st_value = value;
  symtab_base[symtab_count].st_size = 0;
  
  symtab_count = symtab_count + 1;
  return symtab_count - 1;
}

void next()
{
  char *pp;
  long op;
  double frac;
  long sign;
  long exp;
  long start_offset;

  while (tk = *p) {
    ++p;
    if (tk == '\n') {
      if (src) {
        printf("%ld: ", line);
        pp = lp;
        while (pp < p) { printf("%c", *pp); pp = pp + 1; }
        lp = p;
        while (le < e) {
          op = *++le;
          printf("%8.4s", instr_name + op * 5);
          if (op <= ADJ || op == IMMF) printf(" %ld\n", *++le); else printf("\n");
        }
      }
      lp = p;
      ++line;
    }
    else if (tk == '#') {
      while (*p != 0 && *p != '\n') ++p;
    }
    else if ((tk >= 'a' && tk <= 'z') || (tk >= 'A' && tk <= 'Z') || tk == '_') {
      pp = p - 1;
      while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')
        tk = (tk & 0x0000FFFF) * 147 + *p++;
      tk = ((tk & 0x00FFFFFF) << 6) + (p - pp);
      
      id = sym;
      while (id->v_tk) {
        if (tk == id->v_hash && !memcmp(id->v_name, pp, p - pp)) { tk = id->v_tk; return; }
        id = (struct symbol *)((long *)id + Idsz);
      }
      id->v_name = (char *)pp;
      id->v_hash = tk;
      tk = id->v_tk = Id;
      return;
    }
    else if (tk >= '0' && tk <= '9') {
      if (ival = tk - '0') { while (*p >= '0' && *p <= '9') ival = ival * 10 + *p++ - '0'; }
      else if (*p == 'x' || *p == 'X') {
        while ((tk = *++p) && ((tk >= '0' && tk <= '9') || (tk >= 'a' && tk <= 'f') || (tk >= 'A' && tk <= 'F')))
          ival = ival * 16 + (tk & 15) + (tk >= 'A' ? 9 : 0);
      }
      else { while (*p >= '0' && *p <= '7') ival = ival * 8 + *p++ - '0'; }

      if (*p == '.' || *p == 'e' || *p == 'E') {
        fval = (double)ival;
        if (*p == '.') {
          ++p;
          frac = 1.0;
          while (*p >= '0' && *p <= '9') {
            frac = frac * 0.1;
            fval = fval + (double)(*p - '0') * frac;
            ++p;
          }
        }
        if (*p == 'e' || *p == 'E') {
          ++p;
          sign = 1;
          if (*p == '-') { sign = -1; ++p; }
          else if (*p == '+') { ++p; }
          exp = 0;
          while (*p >= '0' && *p <= '9') {
            exp = exp * 10 + (*p - '0');
            ++p;
          }
          if (sign == 1) {
            while (exp > 0) { fval = fval * 10.0; exp = exp - 1; }
          } else {
            while (exp > 0) { fval = fval * 0.1; exp = exp - 1; }
          }
        }
        if (*p == 'f' || *p == 'F') ++p;
        tk = NumF;
      } else {
        if (*p == 'f' || *p == 'F') { ++p; fval = (double)ival; tk = NumF; }
        else tk = Num;
      }
      return;
    }
    else if (tk == '/') {
      if (*p == '/') {
        p = p + 1;
        while (*p != 0 && *p != '\n') p = p + 1;
      }
      else if (*p == '*') {
        p = p + 1;
        while (*p != 0) {
          if (*p == '*' && p[1] == '/') { p = p + 2; break; }
          if (*p == '\n') {
            if (src) {
              printf("%ld: ", line);
              pp = lp;
              while (pp <= p) { printf("%c", *pp); pp = pp + 1; }
            }
            lp = p + 1;
            line = line + 1;
          }
          p = p + 1;
        }
      }
      else { tk = Div; return; }
    }
    else if (tk == '\'' || tk == '"') {
      start_offset = (long)(data - data_base);
      while (*p != 0 && *p != tk) {
        if ((ival = *p++) == '\\') {
          if ((ival = *p++) == 'n') ival = '\n';
        }
        if (tk == '"') *data++ = ival;
      }
      ++p;
      if (tk == '"') {
        *data++ = 0;
        ival = start_offset;
      } else tk = Num;
      return;
    }
    else if (tk == '=') { if (*p == '=') { ++p; tk = Eq; } else tk = Assign; return; }
    else if (tk == '+') { if (*p == '+') { ++p; tk = Inc; } else tk = Add; return; }
    else if (tk == '-') { 
      if (*p == '-') { ++p; tk = Dec; } 
      else if (*p == '>') { ++p; tk = Arrow; }
      else tk = Sub; 
      return; 
    }
    else if (tk == '!') { if (*p == '=') { ++p; tk = Ne; } return; }
    else if (tk == '<') { if (*p == '=') { ++p; tk = Le; } else if (*p == '<') { ++p; tk = Shl; } else tk = Lt; return; }
    else if (tk == '>') { if (*p == '=') { ++p; tk = Ge; } else if (*p == '>') { ++p; tk = Shr; } else tk = Gt; return; }
    else if (tk == '|') { if (*p == '|') { ++p; tk = Lor; } else tk = Or; return; }
    else if (tk == '&') { if (*p == '&') { ++p; tk = Lan; } else tk = And; return; }
    else if (tk == '^') { tk = Xor; return; }
    else if (tk == '%') { tk = Mod; return; }
    else if (tk == '*') { tk = Mul; return; }
    else if (tk == '[') { tk = Brak; return; }
    else if (tk == '?') { tk = Cond; return; }
    else if (tk == '.') { tk = Dot; return; }
    else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || tk == ')' || tk == ']' || tk == ',' || tk == ':') return;
  }
}

void expr(long lev)
{
  long t, *d;
  struct symbol *s;
  long is_dot;

  if (!tk) error("unexpected eof in expression");
  else if (tk == Num) {
    *++e = IMM; *++e = ival; next();
    ty = INT;
  }
  else if (tk == NumF) {
    *++e = IMMF; *++e = double_to_long(fval); next();
    ty = DOUBLE;
  }
  else if (tk == '"') {
    *++e = IMM; *++e = ival; next();
    while (tk == '"') next();
    data = (char *)(((long)data + sizeof(long)) & -sizeof(long)); ty = PTR;
  }
  else if (tk == Sizeof) {
    next(); if (tk == '(') next(); else error("open paren expected in sizeof");
    ty = INT; 
    if (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); ty = INT; }
    else if (tk == Char) { next(); ty = CHAR; }
    else if (tk == Float) { next(); ty = FLOAT; }
    else if (tk == Double) { next(); ty = DOUBLE; }
    else if (tk == Struct) { next(); if (tk == Id) next(); ty = INT; }
    while (tk == Mul) { next(); ty = ty + PTR; }
    if (tk == ')') next(); else error("close paren expected in sizeof");
    *++e = IMM;
    if      (ty == CHAR)   *++e = sizeof(char);
    else                   *++e = sizeof(long);
    ty = INT;
  }
  else if (tk == Id) {
    s = id; next();
    if (tk == '(') {
      next();
      t = 0;
      while (tk != ')') { expr(Assign); *++e = PSH; ++t; if (tk == ',') next(); }
      next();
      if (s->v_class == Sys) *++e = s->v_val;
      else if (s->v_class == Fun) { *++e = JSR; *++e = s->v_val; }
      else error("bad function call");
      if (t) { *++e = ADJ; *++e = t; }
      ty = s->v_type;
    }
    else if (s->v_class == Num) { *++e = IMM; *++e = s->v_val; ty = INT; }
    else {
      if (s->v_class == Loc || s->v_class == LocArr) {
        *++e = LLA; *++e = loc - s->v_val;
        ty = s->v_type;
        if (s->v_class != LocArr) { 
          if (ty == CHAR)       *++e = LC;
          else if (is_float_ty(ty))  *++e = LF;
          else                       *++e = LI;
        }
      }
      else if (s->v_class == GloArr) {
        *++e = IMM; *++e = s->v_val; 
        ty = s->v_type;
      }
      else if (s->v_class == Glo) {
        *++e = IMM; *++e = s->v_val;
        ty = s->v_type;
        if      (ty == CHAR)       *++e = LC;
        else if (is_float_ty(ty))  *++e = LF;
        else                       *++e = LI;
      }
      else error("undefined variable");
    }
  }
  else if (tk == '(') {
    next();
    if (tk == Int || tk == Int64 || tk == Char || tk == Long || tk == Short ||
        tk == Float || tk == Double || tk == Struct) {
      if      (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); t = INT; }
      else if (tk == Char)   { next(); t = CHAR; }
      else if (tk == Float)  { next(); t = FLOAT; }
      else if (tk == Double) { next(); t = DOUBLE; }
      else if (tk == Struct) { next(); if (tk == Id) next(); t = INT; }
      while (tk == Mul) { next(); t = t + PTR; }
      if (tk == ')') next(); else error("bad cast");
      expr(Inc);
      if (is_float_ty(t) && !is_float_ty(ty)) { *++e = ITF; }  
      else if (!is_float_ty(t) && is_float_ty(ty)) { *++e = FTI; } 
      ty = t;
    }
    else {
      expr(Assign);
      if (tk == ')') next(); else error("close paren expected");
    }
  }
  else if (tk == Mul) {
    next(); expr(Inc);
    if (ty > INT) ty = ty - PTR; else error("bad dereference");
    if      (ty == CHAR)      *++e = LC;
    else if (is_float_ty(ty)) *++e = LF;
    else                      *++e = LI;
  }
  else if (tk == And) {
    next(); expr(Inc);
    if (*e == LC || *e == LI || *e == LF) e = e - 1;
    else error("bad address-of");
    ty = ty + PTR;
  }
  else if (tk == '!') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = 0; *++e = EQ; ty = INT; }
  else if (tk == '~') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = -1; *++e = XOR; ty = INT; }
  else if (tk == Add) { next(); expr(Inc); ty = INT; }
  else if (tk == Sub) {
    next();
    if (tk == Num)  { *++e = IMM;  *++e = -ival;                      next(); ty = INT; }
    else if (tk == NumF) { *++e = IMMF; *++e = double_to_long(-fval); next(); ty = DOUBLE; }
    else {
      expr(Inc);
      if (is_float_ty(ty)) {
        *++e = PSH;
        *++e = IMMF; *++e = double_to_long(-1.0);
        *++e = FMUL;
      } else {
        *++e = PSH; *++e = IMM; *++e = -1; *++e = MUL;
        ty = INT;
      }
    }
  }
  else if (tk == Inc || tk == Dec) {
    t = tk; next(); expr(Inc);
    if      (*e == LC) { *e = PSH; *++e = LC; }
    else if (*e == LI) { *e = PSH; *++e = LI; }
    else if (*e == LF) { *e = PSH; *++e = LF; }
    else error("bad lvalue in pre-increment");
    *++e = PSH;
    *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
    *++e = (t == Inc) ? ADD : SUB;
    *++e = (ty == CHAR) ? SC : (is_float_ty(ty) ? SF : SI);
  }
  else error("bad expression");
  
  while (tk >= lev) {
    t = ty;
    if (tk == Assign) {
      next();
      if (*e == LC || *e == LI || *e == LF) *e = PSH;
      else error("bad lvalue in assignment");
      expr(Assign);
      if (is_float_ty(t) && !is_float_ty(ty)) *++e = ITF;
      else if (!is_float_ty(t) && is_float_ty(ty)) *++e = FTI;
      *++e = (t == CHAR) ? SC : (is_float_ty(t) ? SF : SI);
      ty = t;
    }
    else if (tk == Cond) {
      next();
      *++e = BZ; d = ++e;
      expr(Assign);
      if (tk == ':') next(); else error("conditional missing colon");
      *d = (long)(e + 3 - text_base); *++e = JMP; d = ++e;
      expr(Cond);
      *d = (long)(e + 1 - text_base);
    }
    else if (tk == Lor) { next(); *++e = BNZ; d = ++e; expr(Lan); *d = (long)(e + 1 - text_base); ty = INT; }
    else if (tk == Lan) { next(); *++e = BZ;  d = ++e; expr(Or);  *d = (long)(e + 1 - text_base); ty = INT; }
    else if (tk == Or)  { next(); *++e = PSH; expr(Xor); *++e = OR;  ty = INT; }
    else if (tk == Xor) { next(); *++e = PSH; expr(And); *++e = XOR; ty = INT; }
    else if (tk == And) { next(); *++e = PSH; expr(Eq);  *++e = AND; ty = INT; }
    else if (tk == Eq) {
      next(); *++e = PSH; expr(Lt);
      if (is_float_ty(t) || is_float_ty(ty)) {
         if (!is_float_ty(ty)) *++e = ITF;
         if (!is_float_ty(t))  *++e = ITFS;
         *++e = FEQ;
      } else { *++e = EQ; }
      ty = INT;
    }
    else if (tk == Ne) {
      next(); *++e = PSH; expr(Lt);
      if (is_float_ty(t) || is_float_ty(ty)) {
         if (!is_float_ty(ty)) *++e = ITF;
         if (!is_float_ty(t))  *++e = ITFS;
         *++e = FNE;
      } else { *++e = NE; }
      ty = INT;
    }
    else if (tk == Lt) {
      next(); *++e = PSH; expr(Shl);
      if (is_float_ty(t) || is_float_ty(ty)) {
         if (!is_float_ty(ty)) *++e = ITF;
         if (!is_float_ty(t))  *++e = ITFS;
         *++e = FLT;
      } else { *++e = LT; }
      ty = INT;
    }
    else if (tk == Gt) {
      next(); *++e = PSH; expr(Shl);
      if (is_float_ty(t) || is_float_ty(ty)) {
         if (!is_float_ty(ty)) *++e = ITF;
         if (!is_float_ty(t))  *++e = ITFS;
         *++e = FGT;
      } else { *++e = GT; }
      ty = INT;
    }
    else if (tk == Le) {
      next(); *++e = PSH; expr(Shl);
      if (is_float_ty(t) || is_float_ty(ty)) {
         if (!is_float_ty(ty)) *++e = ITF;
         if (!is_float_ty(t))  *++e = ITFS;
         *++e = FLE;
      } else { *++e = LE; }
      ty = INT;
    }
    else if (tk == Ge) {
      next(); *++e = PSH; expr(Shl);
      if (is_float_ty(t) || is_float_ty(ty)) {
         if (!is_float_ty(ty)) *++e = ITF;
         if (!is_float_ty(t))  *++e = ITFS;
         *++e = FGE;
      } else { *++e = GE; }
      ty = INT;
    }
    else if (tk == Shl) { next(); *++e = PSH; expr(Add); *++e = SHL; ty = INT; }
    else if (tk == Shr) { next(); *++e = PSH; expr(Add); *++e = SHR; ty = INT; }
    else if (tk == Add) {
      next(); *++e = PSH; expr(Mul);
      if (is_float_ty(t) || is_float_ty(ty)) {
        if (!is_float_ty(ty)) *++e = ITF;
        if (!is_float_ty(t))  *++e = ITFS;
        *++e = FADD; ty = DOUBLE;
      } else {
        if (t > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = MUL; }
        *++e = ADD; ty = t;
      }
    }
    else if (tk == Sub) {
      next(); *++e = PSH; expr(Mul);
      if (is_float_ty(t) || is_float_ty(ty)) {
        if (!is_float_ty(ty)) *++e = ITF;
        if (!is_float_ty(t))  *++e = ITFS;
        *++e = FSUB; ty = DOUBLE;
      } else {
        if (t > PTR && t == ty) { *++e = SUB; *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = DIV; ty = INT; }
        else if (t > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = MUL; *++e = SUB; ty = t; }
        else { *++e = SUB; ty = t; }
      }
    }
    else if (tk == Mul) {
      next(); *++e = PSH; expr(Inc);
      if (is_float_ty(t) || is_float_ty(ty)) {
        if (!is_float_ty(ty)) *++e = ITF;
        if (!is_float_ty(t))  *++e = ITFS;
        *++e = FMUL; ty = DOUBLE;
      } else { *++e = MUL; ty = INT; }
    }
    else if (tk == Div) {
      next(); *++e = PSH; expr(Inc);
      if (is_float_ty(t) || is_float_ty(ty)) {
        if (!is_float_ty(ty)) *++e = ITF;
        if (!is_float_ty(t))  *++e = ITFS;
        *++e = FDIV; ty = DOUBLE;
      } else { *++e = DIV; ty = INT; }
    }
    else if (tk == Mod) { next(); *++e = PSH; expr(Inc); *++e = MOD; ty = INT; }
    else if (tk == Inc || tk == Dec) {
      if      (*e == LC) { *e = PSH; *++e = LC; }
      else if (*e == LI) { *e = PSH; *++e = LI; }
      else if (*e == LF) { *e = PSH; *++e = LF; }
      else error("bad lvalue in post-increment");
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
      *++e = (tk == Inc) ? ADD : SUB;
      *++e = (ty == CHAR) ? SC : (is_float_ty(ty) ? SF : SI);
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
      *++e = (tk == Inc) ? SUB : ADD;
      next();
    }
    else if (tk == Brak) {
      next(); *++e = PSH; expr(Assign);
      if (tk == ']') next(); else error("close bracket expected");
      if (t > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = MUL; }
      else if (t < PTR) error("pointer type expected");
      *++e = ADD;
      ty = t - PTR;
      if      (ty == CHAR)      *++e = LC;
      else if (is_float_ty(ty)) *++e = LF;
      else                      *++e = LI;
    }
    else if (tk == Arrow || tk == Dot) {
      is_dot = (tk == Dot);
      next();
      if (tk != Id) error("bad struct member");
      if (id->v_class != Member) error("undefined struct member");
      if (is_dot) {
        if (*e == LC || *e == LI || *e == LF) {
          e = e - 1;
          if (le > e) le = e;
        }
        else error("bad lvalue in struct");
      }
      *++e = PSH; *++e = IMM; *++e = id->v_val; *++e = ADD;
      ty = id->v_type;
      if      (ty == CHAR)      *++e = LC;
      else if (is_float_ty(ty)) *++e = LF;
      else                      *++e = LI;
      next();
    }
    else error("compiler error");
  }
}

void stmt()
{
  long *b, *c, *p2;
  long a, d;

  if (tk == If) {
    next();
    if (tk == '(') next(); else error("open paren expected");
    expr(Assign);
    if (tk == ')') next(); else error("close paren expected");
    *++e = BZ; b = ++e;
    stmt();
    if (tk == Else) {
      *b = (long)(e + 3 - text_base); *++e = JMP; b = ++e;
      next();
      stmt();
    }
    *b = (long)(e + 1 - text_base);
  }
  else if (tk == While) {
    next();
    a = (long)(e + 1 - text_base);
    if (tk == '(') next(); else error("open paren expected");
    expr(Assign);
    if (tk == ')') next(); else error("close paren expected");
    *++e = BZ; b = ++e;
    stmt();
    *++e = JMP; *++e = (long)a;
    *b = (long)(e + 1 - text_base);
    p2 = b + 1;
    while (p2 < e) {
      if (*p2 == JMP) {
        ++p2;
        if (*p2 == -1) *p2 = (long)(e + 1 - text_base);
        else if (*p2 == -2) *p2 = (long)a;
      }
      ++p2;
    }
  }
  else if (tk == For) {
    next();
    if (tk == '(') next(); else error("open paren expected");
    if (tk != ';') expr(Assign);
    if (tk == ';') next(); else error("semicolon expected");
    a = (long)(e + 1 - text_base);
    b = 0;
    if (tk != ';') { expr(Assign); *++e = BZ; b = ++e; }
    if (tk == ';') next(); else error("semicolon expected");
    *++e = JMP; c = ++e;
    d = (long)(e + 1 - text_base);
    if (tk != ')') expr(Assign);
    *++e = JMP; *++e = (long)a;
    *c = (long)(e + 1 - text_base);
    if (tk == ')') next(); else error("close paren expected");
    stmt();
    *++e = JMP; *++e = (long)d;
    if (b) *b = (long)(e + 1 - text_base);
    p2 = c + 1;
    while (p2 < e) {
      if (*p2 == JMP) {
        ++p2;
        if (*p2 == -1) *p2 = (long)(e + 1 - text_base);
        else if (*p2 == -2) *p2 = (long)d;
      }
      ++p2;
    }
  }
  else if (tk == Break) {
    next();
    *++e = JMP; *++e = -1;
    if (tk == ';') next(); else error("semicolon expected");
  }
  else if (tk == Continue) {
    next();
    *++e = JMP; *++e = -2;
    if (tk == ';') next(); else error("semicolon expected");
  }
  else if (tk == Return) {
    next();
    if (tk != ';') expr(Assign);
    *++e = LEV;
    if (tk == ';') next(); else error("semicolon expected");
  }
  else if (tk == '{') {
    next();
    while (tk != '}') stmt();
    next();
  }
  else if (tk == ';') {
    next();
  }
  else {
    expr(Assign);
    if (tk == ';') next(); else error("semicolon expected");
  }
}

long prog() {
  long bt, i, offset, mty;
  long *ent_ptr;
  
  while (tk) {
    bt = INT;
    if      (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); bt = INT; }
    else if (tk == Char)   { next(); bt = CHAR; }
    else if (tk == Float)  { next(); bt = FLOAT; }
    else if (tk == Double) { next(); bt = DOUBLE; }
    else if (tk == Struct) {
      next();
      if (tk == Id) next();
      if (tk == '{') {
        next();
        offset = 0;
        while (tk != '}') {
          mty = INT;
          if      (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); mty = INT; }
          else if (tk == Char)   { next(); mty = CHAR; }
          else if (tk == Float)  { next(); mty = FLOAT; }
          else if (tk == Double) { next(); mty = DOUBLE; }
          else if (tk == Struct) { next(); if (tk == Id) next(); mty = INT; }
          while (tk == Mul) { next(); mty = mty + PTR; }
          if (tk != Id) error("bad struct member");
          
          id->v_class = Member;
          id->v_val = offset;
          id->v_type = mty;
          offset = offset + sizeof(long);
          
          next();
          if (tk == ';') next();
        }
        next();
      }
      bt = INT;
    }
    else if (tk == Enum) {
      next();
      if (tk != '{') next();
      if (tk == '{') {
        next();
        i = 0;
        while (tk != '}') {
          if (tk != Id) error("bad enum identifier");
          next();
          if (tk == Assign) {
            next();
            if (tk != Num) error("bad enum initializer");
            i = ival;
            next();
          }
          id->v_class = Num; id->v_type = INT; id->v_val = i++;
          if (tk == ',') next();
        }
        next();
      }
    }
    
    while (tk != ';' && tk != '}') {
      ty = bt;
      while (tk == Mul) { next(); ty = ty + PTR; }
      if (tk != Id) error("bad global declaration");
      if (id->v_class) error("duplicate global definition");
      next();
      id->v_type = ty;
      if (tk == '(') {
        id->v_class = Fun;
        id->v_val = (long)(e + 1 - text_base); 
        add_symbol(id, 0x12, id->v_val);
        
        next(); i = 0;
        while (tk != ')') {
          ty = INT;
          if      (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); ty = INT; }
          else if (tk == Char)   { next(); ty = CHAR; }
          else if (tk == Float)  { next(); ty = FLOAT; }
          else if (tk == Double) { next(); ty = DOUBLE; }
          else if (tk == Struct) { next(); if (tk == Id) next(); ty = INT; }
          while (tk == Mul) { next(); ty = ty + PTR; }
          if (tk != Id) error("bad parameter declaration");
          if (id->v_class == Loc) error("duplicate parameter definition");
          id->v_hclass = id->v_class; id->v_class = Loc;
          id->v_htype  = id->v_type;  id->v_type = ty;
          id->v_hval   = id->v_val;   id->v_val = i++;
          next();
          if (tk == ',') next();
        }
        next();
        if (tk != '{') error("bad function definition");
        loc = ++i;
        next();

        *++e = ENT; ent_ptr = ++e; *ent_ptr = 0;

        while (tk == Int || tk == Int64 || tk == Char || tk == Long || tk == Short ||
               tk == Float || tk == Double || tk == Struct) {
          if      (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); bt = INT; }
          else if (tk == Char)   { next(); bt = CHAR; }
          else if (tk == Float)  { next(); bt = FLOAT; }
          else if (tk == Double) { next(); bt = DOUBLE; }
          else if (tk == Struct) { next(); if (tk == Id) next(); bt = INT; }
          while (tk != ';') {
            ty = bt;
            while (tk == Mul) { next(); ty = ty + PTR; }
            if (tk != Id) error("bad local declaration");
            if (id->v_class == Loc) error("duplicate local definition");
            id->v_hclass = id->v_class; id->v_class = Loc;
            id->v_htype  = id->v_type;  id->v_type = ty;
            id->v_hval   = id->v_val;   id->v_val = ++i;
            next();
            if (tk == Brak) {
              id->v_class = LocArr;
              next();
              if (tk != Num) error("array size must be constant");
              i = i + ival - 1; 
              id->v_val = i;
              id->v_type = ty + PTR;
              next();
              if (tk == ']') next();
            } else if (tk == Assign) {
              next();
              *++e = LLA; *++e = loc - id->v_val;
              *++e = PSH;
              expr(Assign);
              if (is_float_ty(id->v_type) && !is_float_ty(ty)) *++e = ITF;
              else if (!is_float_ty(id->v_type) && is_float_ty(ty)) *++e = FTI;
              *++e = (id->v_type == CHAR) ? SC : (is_float_ty(id->v_type) ? SF : SI);
            }
            if (tk == ',') next();
          }
          next();
        }
        
        *ent_ptr = i - loc;
        
        while (tk != '}') stmt();
        *++e = LEV;
        id = sym;
        while (id->v_tk) {
          if (id->v_class == Loc || id->v_class == LocArr) {
            id->v_class = id->v_hclass;
            id->v_type  = id->v_htype;
            id->v_val   = id->v_hval;
          }
          id = (struct symbol *)((long *)id + Idsz);
        }
      }
      else {
        if (tk == Brak) {
          id->v_class = GloArr;
          id->v_val = (long)(data - data_base);
          add_symbol(id, 0x11, id->v_val);
          
          next();
          if (tk != Num) error("array size must be constant");
          else {
            data = data + ival * sizeof(long);
            next();
          }
          if (tk == ']') next();
          id->v_type = ty + PTR;
        } else {
          id->v_class = Glo;
          id->v_val = (long)(data - data_base); 
          add_symbol(id, 0x11, id->v_val);
          
          data = data + sizeof(long);
          
          if (tk == Assign) {
            next();
            if (tk == Num) { *(long *)(data_base + id->v_val) = ival; next(); }
            else if (tk == NumF) { *(double *)(data_base + id->v_val) = fval; next(); }
            else if (tk == '"') { *(long *)(data_base + id->v_val) = ival; next(); while(tk == '"') next(); }
            else if (tk == '-') {
              next();
              if (tk == Num) { *(long *)(data_base + id->v_val) = -ival; next(); }
              else if (tk == NumF) { *(double *)(data_base + id->v_val) = -fval; next(); }
              else error("bad global initialization");
            }
            else error("bad global initialization");
          }
        }
      }
      if (tk == ',') next();
    }
    next();
  }
  return 0;
}

long to_addr(long addr, long d_base, long poolsz) {
    if (addr >= 0) {
        if (addr < poolsz) return d_base + addr;
    }
    return addr;
}

long run(long *pc, long *bp, long *sp, char *d_base, long *t_base, long poolsz) {
  long a, cycle;
  long i, *t;
  double fa, fb;
  char *fmt;
  long fargs[5];
  char *f2;
  long ai;
  char spec[32];
  char *sp3;
  double dv;
  long cv;
  long t_addr;

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

    else if (i == OPEN) a = open((char *)to_addr(sp[1], (long)d_base, poolsz), *sp);
    else if (i == READ) a = read(sp[2], (char *)to_addr(sp[1], (long)d_base, poolsz), *sp);
    else if (i == CLOS) a = close(*sp);
    else if (i == PRTF) {
      t = sp + pc[1];
      fmt = (char *)to_addr(t[-1], (long)d_base, poolsz);
      fargs[0]=t[-2]; fargs[1]=t[-3]; fargs[2]=t[-4]; fargs[3]=t[-5]; fargs[4]=t[-6];
      f2 = fmt; ai = 0; a = 0;
      while (*f2) {
        if (*f2 != '%') { printf("%c", *f2++); a = a + 1; continue; }
        sp3 = spec;
        *sp3++ = *f2++;
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
    
    else if (i == FOPN) a = (long)fopen((char *)to_addr(sp[1], (long)d_base, poolsz), (char *)to_addr(*sp, (long)d_base, poolsz));
    else if (i == FWRT) a = (long)fwrite((void *)to_addr(sp[3], (long)d_base, poolsz), sp[2], sp[1], (FILE *)*sp);
    else if (i == FCLS) a = (long)fclose((FILE *)*sp);
    
    else { printf("unknown instruction = %ld! cycle = %ld\n", i, cycle); return -1; }
  }
}

// -------------------------------------------------------
// 產生並輸出 ELF 檔案的核心邏輯
// -------------------------------------------------------
void w16(char *buf, long p, long v) { buf[p] = v; buf[p+1] = v>>8; }
void w32(char *buf, long p, long v) { buf[p] = v; buf[p+1] = v>>8; buf[p+2] = v>>16; buf[p+3] = v>>24; }
void w64(char *buf, long p, long v) { w32(buf, p, v); w32(buf, p+4, v>>32); }

void write_elf(char *filename) {
  long f;
  char *hdr;
  long text_sz, data_sz, symtab_sz, strtab_sz, shstrtab_sz;
  long off_hdr, off_shdr, off_shstrtab, off_text, off_data, off_symtab, off_strtab;
  long i, j, k, sym_p;
  char *shstr;
  char *packed_symtab;

  text_sz = (long)(e - text_base) * sizeof(long);
  data_sz = (long)(data - data_base);
  symtab_sz = symtab_count * 24; 
  strtab_sz = strtab_ptr;
  shstr = "\0.text\0.data\0.symtab\0.strtab\0.shstrtab\0";
  shstrtab_sz = 39;

  off_hdr = 0;
  off_shdr = 64;
  off_shstrtab = off_shdr + 6 * 64; // 6 個區段: NULL, .text, .data, .symtab, .strtab, .shstrtab
  off_text = off_shstrtab + shstrtab_sz;
  off_data = off_text + text_sz;
  off_symtab = off_data + data_sz;
  off_strtab = off_symtab + symtab_sz;

  f = (long)fopen(filename, "wb");
  if (!f) { printf("Cannot open output file %s\n", filename); return; }

  hdr = (char *)malloc(off_text);
  memset(hdr, 0, off_text);

  // 1. ELF Header (64 bytes)
  hdr[0] = 0x7f; hdr[1] = 'E'; hdr[2] = 'L'; hdr[3] = 'F';
  hdr[4] = 2; hdr[5] = 1; hdr[6] = 1; 
  w16(hdr, 16, 1);    // e_type = ET_REL (1) -> Object file 格式
  w16(hdr, 18, 0xB7); // e_machine = AArch64 (Apple Silicon). 若為 x86_64 請填 0x3E.
  w32(hdr, 20, 1);    // e_version
  w64(hdr, 24, 0);    // e_entry
  w64(hdr, 32, 0);    // e_phoff
  w64(hdr, 40, off_shdr); // e_shoff
  w32(hdr, 48, 0);    // e_flags
  w16(hdr, 52, 64);   // e_ehsize
  w16(hdr, 54, 0);    // e_phentsize
  w16(hdr, 56, 0);    // e_phnum
  w16(hdr, 58, 64);   // e_shentsize
  w16(hdr, 60, 6);    // e_shnum (總 Section 數)
  w16(hdr, 62, 5);    // e_shstrndx (shstrtab index = 5)

  // 2. Section Headers
  // Shdr 1: .text
  i = off_shdr + 64;
  w32(hdr, i+0, 1); w32(hdr, i+4, 1); // sh_name, sh_type (PROGBITS)
  w64(hdr, i+8, 6); // sh_flags (AX)
  w64(hdr, i+16, 0); // sh_addr
  w64(hdr, i+24, off_text); // sh_offset
  w64(hdr, i+32, text_sz); // sh_size
  w64(hdr, i+48, 8); // sh_addralign

  // Shdr 2: .data
  i = i + 64;
  w32(hdr, i+0, 7); w32(hdr, i+4, 1); // sh_name, sh_type (PROGBITS)
  w64(hdr, i+8, 3); // sh_flags (WA)
  w64(hdr, i+16, 0);
  w64(hdr, i+24, off_data);
  w64(hdr, i+32, data_sz);
  w64(hdr, i+48, 8);

  // Shdr 3: .symtab
  i = i + 64;
  w32(hdr, i+0, 13); w32(hdr, i+4, 2); // sh_name, sh_type (SYMTAB)
  w64(hdr, i+8, 0);
  w64(hdr, i+16, 0);
  w64(hdr, i+24, off_symtab);
  w64(hdr, i+32, symtab_sz);
  w32(hdr, i+40, 4); // sh_link (指向 .strtab)
  w32(hdr, i+44, 0); // sh_info
  w64(hdr, i+48, 8);
  w64(hdr, i+56, 24); // sh_entsize (Elf64_Sym is 24 bytes)

  // Shdr 4: .strtab
  i = i + 64;
  w32(hdr, i+0, 21); w32(hdr, i+4, 3); // sh_name, sh_type (STRTAB)
  w64(hdr, i+8, 0);
  w64(hdr, i+16, 0);
  w64(hdr, i+24, off_strtab);
  w64(hdr, i+32, strtab_sz);
  w64(hdr, i+48, 1);

  // Shdr 5: .shstrtab
  i = i + 64;
  w32(hdr, i+0, 29); w32(hdr, i+4, 3); // sh_name, sh_type (STRTAB)
  w64(hdr, i+8, 0);
  w64(hdr, i+16, 0);
  w64(hdr, i+24, off_shstrtab);
  w64(hdr, i+32, shstrtab_sz);
  w64(hdr, i+48, 1);

  // 3. Copy .shstrtab
  k = 0;
  while (k < shstrtab_sz) { hdr[off_shstrtab + k] = shstr[k]; k = k + 1; }

  // 4. 打包 c4 寬鬆記憶體結構為標準的 24-byte Elf64_Sym
  packed_symtab = (char *)malloc(symtab_sz);
  memset(packed_symtab, 0, symtab_sz);
  j = 0;
  while (j < symtab_count) {
      sym_p = j * 24;
      w32(packed_symtab, sym_p + 0, symtab_base[j].st_name);
      packed_symtab[sym_p + 4] = symtab_base[j].st_info;
      packed_symtab[sym_p + 5] = symtab_base[j].st_other;
      w16(packed_symtab, sym_p + 6, symtab_base[j].st_shndx);
      w64(packed_symtab, sym_p + 8, symtab_base[j].st_value);
      w64(packed_symtab, sym_p + 16, symtab_base[j].st_size);
      j = j + 1;
  }

  // 5. 寫入所有資料
  fwrite(hdr, 1, off_text, (FILE *)f);
  fwrite((void *)text_base, 1, text_sz, (FILE *)f);
  fwrite((void *)data_base, 1, data_sz, (FILE *)f);
  fwrite(packed_symtab, 1, symtab_sz, (FILE *)f);
  fwrite(strtab_base, 1, strtab_sz, (FILE *)f);

  fclose((FILE *)f);
  free(hdr);
  free(packed_symtab);
  printf(">> Successfully written ELF object to %s <<\n", filename);
}

long compile(long argc, char **argv)
{
  long fd, poolsz;
  struct symbol *idmain;
  long *pc, *bp, *sp;
  long i, *t, exit_offset;
  char *outfile;

  instr_name = "LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF,OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,FOPN,FWRT,FCLS,";

  outfile = 0;
  --argc; ++argv;
  while (argc > 0 && **argv == '-') {
    if ((*argv)[1] == 's') { src = 1; --argc; ++argv; }
    else if ((*argv)[1] == 'd') { debug = 1; --argc; ++argv; }
    else if ((*argv)[1] == 'o') { 
      --argc; ++argv; 
      outfile = *argv; 
      --argc; ++argv; 
    }
    else { --argc; ++argv; }
  }

  if (argc < 1) { printf("usage: c4f [-s] [-d] [-o out.elf] file ...\n"); return -1; }

  poolsz = 1024*1024;
  if (!(sym = (struct symbol *)malloc(poolsz))) { printf("could not malloc(%ld) symbol area\n", poolsz); return -1; }
  if (!(le = e = malloc(poolsz)))               { printf("could not malloc(%ld) text area\n",   poolsz); return -1; }
  if (!(data = malloc(poolsz)))                 { printf("could not malloc(%ld) data area\n",   poolsz); return -1; }
  if (!(sp = malloc(poolsz)))                   { printf("could not malloc(%ld) stack area\n",  poolsz); return -1; }
  
  if (!(symtab_base = (struct Elf64_Sym *)malloc(poolsz))) { return -1; }
  if (!(strtab_base = (char *)malloc(poolsz))) { return -1; }

  if ((fd = open(*argv, 0)) < 0) { printf("could not open(%s)\n", *argv); return -1; }

  memset(sym,  0, poolsz);
  memset(e,    0, poolsz);
  memset(data, 0, poolsz);
  
  text_base = e;
  data_base = data;
  symtab_count = 0;
  strtab_ptr = 1;
  strtab_base[0] = 0; 

  // [修改] 新增 void, FILE 關鍵字註冊
  p = "char double else enum float for break continue if int int64 long return short sizeof while struct open read close printf malloc free memset memcmp exit fopen fwrite fclose void FILE main";
  i = Char; while (i <= Struct) { next(); id->v_tk = i++; }
  i = OPEN; while (i <= FCLS) { next(); id->v_class = Sys; id->v_type = INT; id->v_val = i++; }
  next(); id->v_tk = Char; // void -> Char (ptr)
  next(); id->v_tk = Long; // FILE -> Long (ptr)
  next(); idmain = id;     // main

  if (!(lp = p = malloc(poolsz))) { printf("could not malloc(%ld) source area\n", poolsz); return -1; }
  if ((i = read(fd, p, poolsz-1)) <= 0) { printf("read() returned %ld\n", i); return -1; }
  p[i] = 0;
  close(fd);

  line = 1;
  next(); 

  if (prog() == -1) return -1;

  if (!(idmain->v_val)) { printf("main() not defined\n"); return -1; }
  
  *++e = PSH;
  *++e = EXIT;
  exit_offset = (long)(e - 1 - text_base);

  if (outfile) {
    write_elf(outfile);
  }

  if (src) return 0;

  bp = sp = (long *)((long)sp + poolsz);
  *--sp = argc;
  *--sp = (long)argv;
  *--sp = exit_offset;

  return run(text_base + idmain->v_val, bp, sp, data_base, text_base, poolsz);
}

int main(int argc, char **argv) {
  return (int)compile(argc, argv);
}