// c4.c - C in four functions

// char, int, and pointer types
// if, while, return, and expression statements
// just enough features to allow self-compilation and a bit more

// Written by Robert Swierczek
// 修改者: 陳鍾誠 (模組化並加上中文註解)
// 修改者: (新增 struct 支援，並使用 struct 重構符號表以進行自我編譯)
// 修改者: (修復 Sanitizer ASan/UBSan 報錯，修正原版 C4 十年陣列型別 Bug)
 
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

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

// 改用 struct pointer 宣告符號表
struct symbol *sym, *id;

enum {
  Num = 128, Fun, Sys, Glo, Loc, Id, Member,
  Char, Else, Enum, For, Break, Continue, If, Int, Int64, Long, Return, Short, Sizeof, While, Struct,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak, Arrow, Dot
};

enum { LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT };

enum { CHAR, SHORT, INT, LONG, PTR };

// 保留 Idsz 用於指標偏移 (我們 struct 有 9 個成員)
enum { Idsz = 9 };

void next()
{
  char *pp;

  while (tk = *p) {
    ++p;
    if (tk == '\n') {
      if (src) {
        printf("%ld: %.*s", line, (int)(p - lp), lp);
        lp = p;
        while (le < e) {
          printf("%8.4s", &"LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
                           "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                           "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[*++le * 5]);
          if (*le <= ADJ) printf(" %ld\n", *++le); else printf("\n");
        }
      }
      ++line;
    }
    else if (tk == '#') {
      while (*p != 0 && *p != '\n') ++p;
    }
    else if ((tk >= 'a' && tk <= 'z') || (tk >= 'A' && tk <= 'Z') || tk == '_') {
      pp = p - 1;
      // 使用位元遮罩 0x0000FFFF 限制 tk 大小，確保 tk * 147 和 tk << 6 絕對不會發生 Signed Integer Overflow (UB)
      while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')
        tk = (tk & 0x0000FFFF) * 147 + *p++;
      tk = ((tk & 0x00FFFFFF) << 6) + (p - pp);
      
      id = sym;
      while (id->v_tk) {
        if (tk == id->v_hash && !memcmp(id->v_name, pp, p - pp)) { tk = id->v_tk; return; }
        // 透過強制轉型來進行指標運算 (依賴 Idsz 對齊)
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
      tk = Num;
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
    else if (tk == '.') { tk = Dot; return; }
    // 忽略不可見字元與未定義字元
    else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || tk == ')' || tk == ']' || tk == ',' || tk == ':') return;
  }
}

void expr(long lev)
{
  long t, *d;
  struct symbol *s;

  if (!tk) { printf("%ld: unexpected eof in expression\n", line); exit(-1); }
  else if (tk == Num) { *++e = IMM; *++e = ival; next(); ty = INT; }
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
    else if (tk == Struct) { next(); if (tk == Id) next(); ty = INT; }
    while (tk == Mul) { next(); ty = ty + PTR; }
    if (tk == ')') next(); else { printf("%ld: close paren expected in sizeof\n", line); exit(-1); }
    *++e = IMM; *++e = (ty == CHAR) ? sizeof(char) : sizeof(long);
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
      if (s->v_class == Loc) { *++e = LLA; *++e = loc - s->v_val; }
      else if (s->v_class == Glo) { *++e = IMM; *++e = s->v_val; }
      else { printf("%ld: undefined variable\n", line); exit(-1); }
      *++e = ((ty = s->v_type) == CHAR) ? LC : LI;
    }
  }
  else if (tk == '(') {
    next();
    if (tk == Int || tk == Int64 || tk == Char || tk == Long || tk == Short || tk == Struct) {
      if (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); t = INT; }
      else if (tk == Char) { next(); t = CHAR; }
      else if (tk == Struct) { next(); if (tk == Id) next(); t = INT; }
      while (tk == Mul) { next(); t = t + PTR; }
      if (tk == ')') next(); else { printf("%ld: bad cast\n", line); exit(-1); }
      expr(Inc);
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
    *++e = (ty == CHAR) ? LC : LI;
  }
  else if (tk == And) {
    next(); expr(Inc);
    if (*e == LC || *e == LI) --e; else { printf("%ld: bad address-of\n", line); exit(-1); }
    ty = ty + PTR;
  }
  else if (tk == '!') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = 0; *++e = EQ; ty = INT; }
  else if (tk == '~') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = -1; *++e = XOR; ty = INT; }
  else if (tk == Add) { next(); expr(Inc); ty = INT; }
  else if (tk == Sub) {
    next(); *++e = IMM;
    if (tk == Num) { *++e = -ival; next(); } else { *++e = -1; *++e = PSH; expr(Inc); *++e = MUL; }
    ty = INT;
  }
  else if (tk == Inc || tk == Dec) {
    t = tk; next(); expr(Inc);
    if (*e == LC) { *e = PSH; *++e = LC; }
    else if (*e == LI) { *e = PSH; *++e = LI; }
    else { printf("%ld: bad lvalue in pre-increment\n", line); exit(-1); }
    *++e = PSH;
    *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
    *++e = (t == Inc) ? ADD : SUB;
    *++e = (ty == CHAR) ? SC : SI;
  }
  else { printf("%ld: bad expression\n", line); exit(-1); }
  
  while (tk >= lev) {
    t = ty;
    if (tk == Assign) {
      next();
      if (*e == LC || *e == LI) *e = PSH; else { printf("%ld: bad lvalue in assignment\n", line); exit(-1); }
      expr(Assign); *++e = ((ty = t) == CHAR) ? SC : SI;
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
    else if (tk == Eq)  { next(); *++e = PSH; expr(Lt);  *++e = EQ;  ty = INT; }
    else if (tk == Ne)  { next(); *++e = PSH; expr(Lt);  *++e = NE;  ty = INT; }
    else if (tk == Lt)  { next(); *++e = PSH; expr(Shl); *++e = LT;  ty = INT; }
    else if (tk == Gt)  { next(); *++e = PSH; expr(Shl); *++e = GT;  ty = INT; }
    else if (tk == Le)  { next(); *++e = PSH; expr(Shl); *++e = LE;  ty = INT; }
    else if (tk == Ge)  { next(); *++e = PSH; expr(Shl); *++e = GE;  ty = INT; }
    else if (tk == Shl) { next(); *++e = PSH; expr(Add); *++e = SHL; ty = INT; }
    else if (tk == Shr) { next(); *++e = PSH; expr(Add); *++e = SHR; ty = INT; }
    else if (tk == Add) {
      next(); *++e = PSH; expr(Mul);
      if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = MUL;  }
      *++e = ADD;
    }
    else if (tk == Sub) {
      next(); *++e = PSH; expr(Mul);
      if (t > PTR && t == ty) { *++e = SUB; *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = DIV; ty = INT; }
      else if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = MUL; *++e = SUB; }
      else *++e = SUB;
    }
    else if (tk == Mul) { next(); *++e = PSH; expr(Inc); *++e = MUL; ty = INT; }
    else if (tk == Div) { next(); *++e = PSH; expr(Inc); *++e = DIV; ty = INT; }
    else if (tk == Mod) { next(); *++e = PSH; expr(Inc); *++e = MOD; ty = INT; }
    else if (tk == Inc || tk == Dec) {
      if (*e == LC) { *e = PSH; *++e = LC; }
      else if (*e == LI) { *e = PSH; *++e = LI; }
      else { printf("%ld: bad lvalue in post-increment\n", line); exit(-1); }
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
      *++e = (tk == Inc) ? ADD : SUB;
      *++e = (ty == CHAR) ? SC : SI;
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(long) : sizeof(char);
      *++e = (tk == Inc) ? SUB : ADD;
      next();
    }
    else if (tk == Brak) {
      next(); *++e = PSH; expr(Assign);
      if (tk == ']') next(); else { printf("%ld: close bracket expected\n", line); exit(-1); }
      if (t > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(long); *++e = MUL;  }
      else if (t < PTR) { printf("%ld: pointer type expected\n", line); exit(-1); }
      *++e = ADD;
      // [關鍵修復] 原版 C4 忘了把型別降級為元素型別，導致陣列寫入永遠是 8 bytes 的 UB，現在修復了！
      ty = t - PTR;
      *++e = (ty == CHAR) ? LC : LI; 
    }
    else if (tk == Arrow || tk == Dot) {
      next();
      if (tk != Id) { printf("%ld: bad struct member\n", line); exit(-1); }
      if (id->v_class != Member) { printf("%ld: undefined struct member\n", line); exit(-1); }
      *++e = PSH; *++e = IMM; *++e = id->v_val; *++e = ADD;
      ty = id->v_type;
      *++e = (ty == CHAR) ? LC : LI;
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
    // 回填 break(-1) / continue(-2)：只掃 JMP 指令的操作數
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
    // for (init; cond; post) body
    // 結構: init; [cond_start] cond BZ exit;
    //        JMP body; [post_start] post; JMP cond_start;
    //        [body_start] body; JMP post_start; [exit]
    next();
    if (tk == '(') next(); else { printf("%ld: open paren expected\n", line); exit(-1); }
    // init
    if (tk != ';') expr(Assign);
    if (tk == ';') next(); else { printf("%ld: semicolon expected\n", line); exit(-1); }
    // cond_start
    a = e + 1;
    b = 0;
    if (tk != ';') { expr(Assign); *++e = BZ; b = ++e; }
    if (tk == ';') next(); else { printf("%ld: semicolon expected\n", line); exit(-1); }
    // JMP over post, into body
    *++e = JMP; c = ++e;
    // post_start
    d = e + 1;
    if (tk != ')') expr(Assign);
    *++e = JMP; *++e = (long)a;
    // body_start
    *c = (long)(e + 1);
    if (tk == ')') next(); else { printf("%ld: close paren expected\n", line); exit(-1); }
    stmt();
    // jump to post
    *++e = JMP; *++e = (long)d;
    // patch BZ exit
    if (b) *b = (long)(e + 1);
    // 回填 break(-1) / continue(-2)：只掃 JMP 操作數
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
  // 將所有變數移至函式最開頭宣告，以符合 C4 的解析限制
  long bt, i, offset, mty;
  
  while (tk) {
    bt = INT;
    if (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); bt = INT; }
    else if (tk == Char) { next(); bt = CHAR; }
    else if (tk == Struct) {
      next();
      if (tk == Id) next(); // 略過 struct name
      if (tk == '{') {
        next();
        offset = 0; // 偏移量重置
        while (tk != '}') {
          mty = INT;
          if (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); mty = INT; }
          else if (tk == Char) { next(); mty = CHAR; }
          else if (tk == Struct) { next(); if (tk == Id) next(); mty = INT; }
          while (tk == Mul) { next(); mty = mty + PTR; } // 支援指標運算
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
          if (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); ty = INT; }
          else if (tk == Char) { next(); ty = CHAR; }
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
        while (tk == Int || tk == Int64 || tk == Char || tk == Long || tk == Short || tk == Struct) {
          if (tk == Int || tk == Int64 || tk == Long || tk == Short) { next(); bt = INT; }
          else if (tk == Char) { next(); bt = CHAR; }
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
            if (tk == ',') next();
          }
          next();
        }
        *++e = ENT; *++e = i - loc;
        while (tk != '}') stmt();
        *++e = LEV;
        id = sym;
        while (id->v_tk) {
          if (id->v_class == Loc) {
            id->v_class = id->v_hclass;
            id->v_type = id->v_htype;
            id->v_val = id->v_hval;
          }
          id = (struct symbol *)((long *)id + Idsz);
        }
      }
      else {
        id->v_class = Glo;
        id->v_val = (long)data;
        data = data + sizeof(long);
      }
      if (tk == ',') next();
    }
    next();
  }
  return 0;
}

long run(long *pc, long *bp, long *sp) {
  long a, cycle;
  long i, *t;

  cycle = 0;
  while (1) {
    i = *pc++; ++cycle;
    if (debug) {
      printf("%ld> %.4s", cycle,
        &"LLA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
         "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
         "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[i * 5]);
      if (i <= ADJ) printf(" %ld\n", *pc); else printf("\n");
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

    else if (i == OPEN) a = open((char *)sp[1], *sp);
    else if (i == READ) a = read(sp[2], (char *)sp[1], *sp);
    else if (i == CLOS) a = close(*sp);
    else if (i == PRTF) { t = sp + pc[1]; a = printf((char *)t[-1], t[-2], t[-3], t[-4], t[-5], t[-6]); }
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

  --argc; ++argv;
  if (argc > 0 && **argv == '-' && (*argv)[1] == 's') { src = 1; --argc; ++argv; }
  if (argc > 0 && **argv == '-' && (*argv)[1] == 'd') { debug = 1; --argc; ++argv; }
  if (argc < 1) { printf("usage: c4 [-s] [-d] file ...\n"); return -1; }

  // 加大 poolsz，確保處理 struct 時 AST 容量充裕
  poolsz = 1024*1024;
  if (!(sym = (struct symbol *)malloc(poolsz))) { printf("could not malloc(%ld) symbol area\n", poolsz); return -1; }
  if (!(le = e = malloc(poolsz))) { printf("could not malloc(%ld) text area\n", poolsz); return -1; }
  if (!(data = malloc(poolsz))) { printf("could not malloc(%ld) data area\n", poolsz); return -1; }
  if (!(sp = malloc(poolsz))) { printf("could not malloc(%ld) stack area\n", poolsz); return -1; }

  // C4 設計為編譯「單一」主要檔案，剩下的 argv 當成引數傳遞給執行中的程式
  if ((fd = open(*argv, 0)) < 0) { printf("could not open(%s)\n", *argv); return -1; }

  memset(sym,  0, poolsz);
  memset(e,    0, poolsz);
  memset(data, 0, poolsz);

  p = "char else enum for break continue if int int64 long return short sizeof while struct "
      "open read close printf malloc free memset memcmp exit void main";
  i = Char; while (i <= Struct) { next(); id->v_tk = i++; }
  i = OPEN; while (i <= EXIT) { next(); id->v_class = Sys; id->v_type = INT; id->v_val = i++; }
  next(); id->v_tk = Char;
  next(); idmain = id;

  if (!(lp = p = malloc(poolsz))) { printf("could not malloc(%ld) source area\n", poolsz); return -1; }
  if ((i = read(fd, p, poolsz-1)) <= 0) { printf("read() returned %ld\n", i); return -1; }
  p[i] = 0;
  close(fd);

  line = 1;
  next(); 

  if (prog() == -1) return -1;

  if (!(pc = (long *)idmain->v_val)) { printf("main() not defined\n"); return -1; }
  if (src) return 0;

  // 設置堆疊並將剩餘的 arguments 傳遞給被編譯出來的 main 函式
  bp = sp = (long *)((long)sp + poolsz);
  *--sp = EXIT;
  *--sp = PSH; t = sp;
  *--sp = argc;
  *--sp = (long)argv;
  *--sp = (long)t;
  return run(pc, bp, sp);
}

int main(int argc, char **argv) {
  return compile(argc, argv);
}