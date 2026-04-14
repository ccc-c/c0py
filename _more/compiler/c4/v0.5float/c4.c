// c4_float.c - C in four functions (with float/double extension)
//
// char, int, long, float, double, and pointer types
// if, while, for, return, and expression statements
// just enough features to allow self-compilation and a bit more
//
// Written by Robert Swierczek
// 修改者: 陳鍾誠 (模組化並加上中文註解)
// 修改者: (新增 struct 支援，並使用 struct 重構符號表以進行自我編譯)
// 修改者: (修復 Sanitizer ASan/UBSan 報錯，修正原版 C4 十年陣列型別 Bug)
// 修改者: (新增 float/double 型態與浮點 VM 指令集)
// 修復者: (解決 struct 存取 Null 指標 Bug、修復整數浮點混合運算、完美支援自我編譯)

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <math.h>  // 浮點數運算需要

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

// Use long for internal operations (64-bit on 64-bit systems)
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

// 浮點數字面值：使用 union 將 double bits 存入 long
double fval;

// 改用 struct pointer 宣告符號表
struct symbol *sym, *id;

enum {
  Num = 128, NumF, Fun, Sys, Glo, GloArr, Loc, LocArr, Id, Member,
  Char, Double, Else, Enum, Float, For, Break, Continue, If, Int, Int64, Long, Return, Short, Sizeof, While, Struct,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak, Arrow, Dot
};

// VM 指令集：先是整數指令，再追加浮點指令
enum { LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       // --- 浮點擴充指令 ---
       LF  ,                         // 載入 double: a = *(double*)a
       SF  ,                         // 儲存 double: *(double*)*sp++ = *(double*)&a
       IMMF,                         // 立即 double: 下一個 long 解釋為 double bits
       ITF ,                         // int  -> double
       ITFS,                         // int  -> double (將堆疊頂端的整數轉為 double)
       FTI ,                         // double -> int (long)
       FADD, FSUB, FMUL, FDIV,       // 浮點四則 (操作數從堆疊)
       FEQ , FNE , FLT , FGT , FLE , FGE , // 浮點比較
       PRTF_DBL,                     // 特殊 printf 支援 (保留，目前 PRTF 已可用)
       // --- 系統呼叫 ---
       OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT };

// 型別系統：增加 FLOAT / DOUBLE
enum { CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, PTR };

// 保留 Idsz 用於指標偏移 (我們 struct 有 9 個成員)
enum { Idsz = 9 };

// -------------------------------------------------------
// 輔助函式：讓 double 和 long 共用同一塊記憶體
// (C4 不支援 static, inline, union, __builtin_memcpy，故使用指標轉型)
// -------------------------------------------------------
double long_to_double(long v) { return *(double *)&v; }
long   double_to_long(double d) { return *(long *)&d; }

// -------------------------------------------------------
// 指令名稱表（用於 -s / -d 輸出）
// -------------------------------------------------------
// C4 不支援全域變數初始化，故此字串將在 main(compile) 中被初始化
char *instr_name;

#define LAST_WITH_OPERAND ADJ

// 判斷型別是否為浮點（FLOAT 或 DOUBLE）
int is_float_ty(long t) { return (t == FLOAT || t == DOUBLE); }

void next()
{
  char *pp;
  long op;
  double frac;
  long sign;
  long exp;

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
        ++p;
        while (*p != 0 && *p != '\n') ++p;
      }
      else { tk = Div; return; }
    }
    else if (tk == '\'' || tk == '"') {
      pp = data;
      while (*p != 0 && *p != tk) {
        if ((ival = *p++) == '\\') {
          if ((ival = *p++) == 'n') ival = '\n';
        }
        if (tk == '"') *data++ = ival;
      }
      ++p;
      if (tk == '"') ival = (long)pp; else tk = Num;
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
    else if (tk == '.') { 
      tk = Dot; return; 
    }
    else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || tk == ')' || tk == ']' || tk == ',' || tk == ':') return;
  }
}

void expr(long lev)
{
  long t, *d;
  struct symbol *s;
  long is_dot;

  if (!tk) { printf("%ld: unexpected eof in expression\n", line); exit(-1); }
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
    next(); if (tk == '(') next(); else { printf("%ld: open paren expected in sizeof\n", line); exit(-1); }
    ty = INT; 
    if (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); ty = INT; }
    else if (tk == Char) { next(); ty = CHAR; }
    else if (tk == Float) { next(); ty = FLOAT; }
    else if (tk == Double) { next(); ty = DOUBLE; }
    else if (tk == Struct) { next(); if (tk == Id) next(); ty = INT; }
    while (tk == Mul) { next(); ty = ty + PTR; }
    if (tk == ')') next(); else { printf("%ld: close paren expected in sizeof\n", line); exit(-1); }
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
      else { printf("%ld: bad function call\n", line); exit(-1); }
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
      else { printf("%ld: undefined variable\n", line); exit(-1); }
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
      if (tk == ')') next(); else { printf("%ld: bad cast\n", line); exit(-1); }
      expr(Inc);
      if (is_float_ty(t) && !is_float_ty(ty)) { *++e = ITF; }  
      else if (!is_float_ty(t) && is_float_ty(ty)) { *++e = FTI; } 
      ty = t;
    }
    else {
      expr(Assign);
      if (tk == ')') next(); else { printf("%ld: close paren expected\n", line); exit(-1); }
    }
  }
  else if (tk == Mul) {
    next(); expr(Inc);
    if (ty > INT) ty = ty - PTR; else { printf("%ld: bad dereference\n", line); exit(-1); }
    if      (ty == CHAR)      *++e = LC;
    else if (is_float_ty(ty)) *++e = LF;
    else                      *++e = LI;
  }
  else if (tk == And) {
    next(); expr(Inc);
    if (*e == LC || *e == LI || *e == LF) e = e - 1;
    else { printf("%ld: bad address-of\n", line); exit(-1); }
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
    else { printf("%ld: bad lvalue in pre-increment\n", line); exit(-1); }
    *++e = PSH;
    *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
    *++e = (t == Inc) ? ADD : SUB;
    *++e = (ty == CHAR) ? SC : (is_float_ty(ty) ? SF : SI);
  }
  else { printf("%ld: bad expression\n", line); exit(-1); }
  
  while (tk >= lev) {
    t = ty;
    if (tk == Assign) {
      next();
      if (*e == LC || *e == LI || *e == LF) *e = PSH;
      else { printf("%ld: bad lvalue in assignment\n", line); exit(-1); }
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
      if (tk == ':') next(); else { printf("%ld: conditional missing colon\n", line); exit(-1); }
      *d = (long)(e + 3); *++e = JMP; d = ++e;
      expr(Cond);
      *d = (long)(e + 1);
    }
    else if (tk == Lor) { next(); *++e = BNZ; d = ++e; expr(Lan); *d = (long)(e + 1); ty = INT; }
    else if (tk == Lan) { next(); *++e = BZ;  d = ++e; expr(Or);  *d = (long)(e + 1); ty = INT; }
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
      } else {
        *++e = MUL; ty = INT;
      }
    }
    else if (tk == Div) {
      next(); *++e = PSH; expr(Inc);
      if (is_float_ty(t) || is_float_ty(ty)) {
        if (!is_float_ty(ty)) *++e = ITF;
        if (!is_float_ty(t))  *++e = ITFS;
        *++e = FDIV; ty = DOUBLE;
      } else {
        *++e = DIV; ty = INT;
      }
    }
    else if (tk == Mod) { next(); *++e = PSH; expr(Inc); *++e = MOD; ty = INT; }
    else if (tk == Inc || tk == Dec) {
      if      (*e == LC) { *e = PSH; *++e = LC; }
      else if (*e == LI) { *e = PSH; *++e = LI; }
      else if (*e == LF) { *e = PSH; *++e = LF; }
      else { printf("%ld: bad lvalue in post-increment\n", line); exit(-1); }
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
      *++e = (tk == Inc) ? ADD : SUB;
      *++e = (ty == CHAR) ? SC : (is_float_ty(ty) ? SF : SI);
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
      *++e = (tk == Inc) ? SUB : ADD;
      next();
    }
    else if (tk == Brak) {
      next(); *++e = PSH; expr(Assign);
      if (tk == ']') next(); else { printf("%ld: close bracket expected\n", line); exit(-1); }
      if (t > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = MUL; }
      else if (t < PTR) { printf("%ld: pointer type expected\n", line); exit(-1); }
      *++e = ADD;
      ty = t - PTR;
      if      (ty == CHAR)      *++e = LC;
      else if (is_float_ty(ty)) *++e = LF;
      else                      *++e = LI;
    }
    else if (tk == Arrow || tk == Dot) {
      is_dot = (tk == Dot);
      next();
      if (tk != Id) { printf("%ld: bad struct member\n", line); exit(-1); }
      if (id->v_class != Member) { printf("%ld: undefined struct member\n", line); exit(-1); }
      if (is_dot) {
        if (*e == LC || *e == LI || *e == LF) {
          e = e - 1;
          if (le > e) le = e;
        }
        else { printf("%ld: bad lvalue in struct\n", line); exit(-1); }
      }
      *++e = PSH; *++e = IMM; *++e = id->v_val; *++e = ADD;
      ty = id->v_type;
      if      (ty == CHAR)      *++e = LC;
      else if (is_float_ty(ty)) *++e = LF;
      else                      *++e = LI;
      next();
    }
    else { printf("%ld: compiler error tk=%ld\n", line, tk); exit(-1); }
  }
}

void stmt()
{
  long *a, *b, *c, *d, *p2;

  if (tk == If) {
    next();
    if (tk == '(') next(); else { printf("%ld: open paren expected\n", line); exit(-1); }
    expr(Assign);
    if (tk == ')') next(); else { printf("%ld: close paren expected\n", line); exit(-1); }
    *++e = BZ; b = ++e;
    stmt();
    if (tk == Else) {
      *b = (long)(e + 3); *++e = JMP; b = ++e;
      next();
      stmt();
    }
    *b = (long)(e + 1);
  }
  else if (tk == While) {
    next();
    a = e + 1;
    if (tk == '(') next(); else { printf("%ld: open paren expected\n", line); exit(-1); }
    expr(Assign);
    if (tk == ')') next(); else { printf("%ld: close paren expected\n", line); exit(-1); }
    *++e = BZ; b = ++e;
    stmt();
    *++e = JMP; *++e = (long)a;
    *b = (long)(e + 1);
    p2 = b + 1;
    while (p2 < e) {
      if (*p2 == JMP) {
        ++p2;
        if (*p2 == -1) *p2 = (long)(e + 1);
        else if (*p2 == -2) *p2 = (long)a;
      }
      ++p2;
    }
  }
  else if (tk == For) {
    next();
    if (tk == '(') next(); else { printf("%ld: open paren expected\n", line); exit(-1); }
    if (tk != ';') expr(Assign);
    if (tk == ';') next(); else { printf("%ld: semicolon expected\n", line); exit(-1); }
    a = e + 1;
    b = 0;
    if (tk != ';') { expr(Assign); *++e = BZ; b = ++e; }
    if (tk == ';') next(); else { printf("%ld: semicolon expected\n", line); exit(-1); }
    *++e = JMP; c = ++e;
    d = e + 1;
    if (tk != ')') expr(Assign);
    *++e = JMP; *++e = (long)a;
    *c = (long)(e + 1);
    if (tk == ')') next(); else { printf("%ld: close paren expected\n", line); exit(-1); }
    stmt();
    *++e = JMP; *++e = (long)d;
    if (b) *b = (long)(e + 1);
    p2 = c + 1;
    while (p2 < e) {
      if (*p2 == JMP) {
        ++p2;
        if (*p2 == -1) *p2 = (long)(e + 1);
        else if (*p2 == -2) *p2 = (long)d;
      }
      ++p2;
    }
  }
  else if (tk == Break) {
    next();
    *++e = JMP; *++e = -1;
    if (tk == ';') next(); else { printf("%ld: semicolon expected\n", line); exit(-1); }
  }
  else if (tk == Continue) {
    next();
    *++e = JMP; *++e = -2;
    if (tk == ';') next(); else { printf("%ld: semicolon expected\n", line); exit(-1); }
  }
  else if (tk == Return) {
    next();
    if (tk != ';') expr(Assign);
    *++e = LEV;
    if (tk == ';') next(); else { printf("%ld: semicolon expected\n", line); exit(-1); }
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
    if (tk == ';') next(); else { printf("%ld: semicolon expected\n", line); exit(-1); }
  }
}

long prog() {
  long bt, i, offset, mty;
  
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
          if (tk != Id) { printf("%ld: bad struct member\n", line); return -1; }
          
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
          if (tk != Id) { printf("%ld: bad enum identifier %ld\n", line, tk); return -1; }
          next();
          if (tk == Assign) {
            next();
            if (tk != Num) { printf("%ld: bad enum initializer\n", line); return -1; }
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
      if (tk != Id) { printf("%ld: bad global declaration\n", line); return -1; }
      if (id->v_class) { printf("%ld: duplicate global definition\n", line); return -1; }
      next();
      id->v_type = ty;
      if (tk == '(') {
        id->v_class = Fun;
        id->v_val = (long)(e + 1);
        next(); i = 0;
        while (tk != ')') {
          ty = INT;
          if      (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); ty = INT; }
          else if (tk == Char)   { next(); ty = CHAR; }
          else if (tk == Float)  { next(); ty = FLOAT; }
          else if (tk == Double) { next(); ty = DOUBLE; }
          else if (tk == Struct) { next(); if (tk == Id) next(); ty = INT; }
          while (tk == Mul) { next(); ty = ty + PTR; }
          if (tk != Id) { printf("%ld: bad parameter declaration\n", line); return -1; }
          if (id->v_class == Loc) { printf("%ld: duplicate parameter definition\n", line); return -1; }
          id->v_hclass = id->v_class; id->v_class = Loc;
          id->v_htype  = id->v_type;  id->v_type = ty;
          id->v_hval   = id->v_val;   id->v_val = i++;
          next();
          if (tk == ',') next();
        }
        next();
        if (tk != '{') { printf("%ld: bad function definition\n", line); return -1; }
        loc = ++i;
        next();
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
            if (tk != Id) { printf("%ld: bad local declaration\n", line); return -1; }
            if (id->v_class == Loc) { printf("%ld: duplicate local definition\n", line); return -1; }
            id->v_hclass = id->v_class; id->v_class = Loc;
            id->v_htype  = id->v_type;  id->v_type = ty;
            id->v_hval   = id->v_val;   id->v_val = ++i;
            next();
            if (tk == Brak) {
              id->v_class = LocArr;
              next();
              if (tk != Num) { printf("%ld: array size must be constant\n", line); return -1; }
              // C4 不支援 +=，使用 i = i + ...
              i = i + ival - 1; 
              id->v_val = i;
              id->v_type = ty + PTR;
              next();
              if (tk == ']') next();
            }
            if (tk == ',') next();
          }
          next();
        }
        *++e = ENT; *++e = i - loc;
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
          id->v_val = (long)data;
          next();
          if (tk != Num) { printf("%ld: array size must be constant\n", line); }
          else {
            data = data + ival * sizeof(long);
            next();
          }
          if (tk == ']') next();
          id->v_type = ty + PTR;
        } else {
          id->v_class = Glo;
          id->v_val = (long)data;
          data = data + sizeof(long);
        }
      }
      if (tk == ',') next();
    }
    next();
  }
  return 0;
}

// -------------------------------------------------------
// 虛擬機執行器（含浮點指令）
// -------------------------------------------------------
long run(long *pc, long *bp, long *sp) {
  long a, cycle;
  long i, *t;
  double fa, fb;
  // C4 只能將局部變數宣告在函式開頭
  char *fmt;
  long fargs[5];
  char *f2;
  long ai;
  char spec[32];
  char *sp3;
  double dv;
  long cv;

  cycle = 0;
  while (1) {
    i = *pc++; ++cycle;
    if (debug) {
      printf("%ld> %.4s", cycle, instr_name + i * 5);
      if (i <= ADJ || i == IMMF) printf(" %ld\n", *pc); else printf("\n");
    }

    if      (i == LLA) a = (long)(bp + *pc++);
    else if (i == IMM) a = *pc++;
    else if (i == JMP) pc = (long *)*pc;
    else if (i == JSR) { *--sp = (long)(pc + 1); pc = (long *)*pc; }
    else if (i == BZ)  pc = a ? pc + 1 : (long *)*pc;
    else if (i == BNZ) pc = a ? (long *)*pc : pc + 1;
    else if (i == ENT) { *--sp = (long)bp; bp = sp; sp = sp - *pc++; }
    else if (i == ADJ) sp = sp + *pc++;
    else if (i == LEV) { sp = bp; bp = (long *)*sp++; pc = (long *)*sp++; }
    else if (i == LI)  a = *(long *)a;
    else if (i == LC)  a = *(char *)a;
    else if (i == SI)  *(long *)*sp++ = a;
    else if (i == SC)  a = *(char *)*sp++ = a;
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
    else if (i == MUL) a = *sp++ *  a;
    else if (i == DIV) a = *sp++ /  a;
    else if (i == MOD) a = *sp++ %  a;

    else if (i == IMMF) { a = *pc++; } 
    else if (i == LF) { a = double_to_long(*(double *)a); }
    else if (i == SF) { *(double *)*sp++ = long_to_double(a); }
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

    else if (i == OPEN) a = open((char *)sp[1], *sp);
    else if (i == READ) a = read(sp[2], (char *)sp[1], *sp);
    else if (i == CLOS) a = close(*sp);
    else if (i == PRTF) {
      t = sp + pc[1];
      fmt = (char *)t[-1];
      fargs[0]=t[-2]; fargs[1]=t[-3]; fargs[2]=t[-4]; fargs[3]=t[-5]; fargs[4]=t[-6];
      f2 = fmt;
      ai = 0;
      a = 0;
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
        } else {
          a = a + printf(spec, fargs[ai<5?ai:4]);
        }
        if (cv != '%') ai = ai + 1;
      }
    }
    else if (i == MALC) a = (long)malloc(*sp);
    else if (i == FREE) free((void *)*sp);
    else if (i == MSET) a = (long)memset((char *)sp[2], sp[1], *sp);
    else if (i == MCMP) a = memcmp((char *)sp[2], (char *)sp[1], *sp);
    else if (i == EXIT) { 
      if (debug) printf("exit(%ld) cycle = %ld\n", *sp, cycle); 
      return *sp; 
    }
    else { printf("unknown instruction = %ld! cycle = %ld\n", i, cycle); return -1; }
  }
}

long compile(long argc, char **argv)
{
  long fd, poolsz;
  struct symbol *idmain;
  long *pc, *bp, *sp;
  long i, *t;

  // C4 不支援相鄰字串自動合併 (String Concatenation)，必須寫成單一字串
  instr_name = "LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,LF  ,SF  ,IMMF,ITF ,ITFS,FTI ,FADD,FSUB,FMUL,FDIV,FEQ ,FNE ,FLT ,FGT ,FLE ,FGE ,PRTF,OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,";

  --argc; ++argv;
  if (argc > 0 && **argv == '-' && (*argv)[1] == 's') { src = 1; --argc; ++argv; }
  if (argc > 0 && **argv == '-' && (*argv)[1] == 'd') { debug = 1; --argc; ++argv; }
  if (argc < 1) { printf("usage: c4f [-s] [-d] file ...\n"); return -1; }

  poolsz = 1024*1024;
  if (!(sym = (struct symbol *)malloc(poolsz))) { printf("could not malloc(%ld) symbol area\n", poolsz); return -1; }
  if (!(le = e = malloc(poolsz)))               { printf("could not malloc(%ld) text area\n",   poolsz); return -1; }
  if (!(data = malloc(poolsz)))                 { printf("could not malloc(%ld) data area\n",   poolsz); return -1; }
  if (!(sp = malloc(poolsz)))                   { printf("could not malloc(%ld) stack area\n",  poolsz); return -1; }

  if ((fd = open(*argv, 0)) < 0) { printf("could not open(%s)\n", *argv); return -1; }

  memset(sym,  0, poolsz);
  memset(e,    0, poolsz);
  memset(data, 0, poolsz);

  // 關鍵字表：新增 double / float
  p = "char double else enum float for break continue if int int64 long return short sizeof while struct open read close printf malloc free memset memcmp exit void main";
  i = Char; while (i <= Struct) { next(); id->v_tk = i++; }
  // 系統呼叫
  i = OPEN; while (i <= EXIT) { next(); id->v_class = Sys; id->v_type = INT; id->v_val = i++; }
  next(); id->v_tk = Char;   // void -> Char (略過)
  next(); idmain = id;       // main

  if (!(lp = p = malloc(poolsz))) { printf("could not malloc(%ld) source area\n", poolsz); return -1; }
  if ((i = read(fd, p, poolsz-1)) <= 0) { printf("read() returned %ld\n", i); return -1; }
  p[i] = 0;
  close(fd);

  line = 1;
  next(); 

  if (prog() == -1) return -1;

  if (!(pc = (long *)idmain->v_val)) { printf("main() not defined\n"); return -1; }
  if (src) return 0;

  bp = sp = (long *)((long)sp + poolsz);
  *--sp = EXIT;
  *--sp = PSH; t = sp;
  *--sp = argc;
  *--sp = (long)argv;
  *--sp = (long)t;
  return run(pc, bp, sp);
}

int main(int argc, char **argv) {
  return (int)compile(argc, argv);
}