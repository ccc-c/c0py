# c0py -- RISCV 上，由 C+Python 打造的最小電腦環境

c0py 包含

1. xv7 -- 一個由 xv6 延伸修改過來的作業系統
2. c0c 編譯器工具鏈
    * 類似 LLVM，使用 .ll 當中間碼
    * .c => .ll => .o 執行
    * .o 可能是 RISCV 或宿主機 host 上的目的檔
3. py0c 編譯器與 py0i 解譯器工具鏈
    * 使用 .qd 當高層動態語言中間碼
    * .py => .qd =>  .ll => .o 執行

## 編譯器流程

自製 C 語言編譯器 [c0c] 的使用流程

```sh
$ c0c fact.c -o fact.ll # 編譯 fact.c 為 fact.ll
$ ll0c fact.ll -o fact.o # 將 fact.ll 轉換為 RISC-V 上的目的檔
$ rv0vm fact.o # RISC-V 虛擬機 rv0vm 執行 fact.o 
```

自製 Python 語言編譯器 [py0c] 的使用流程

```sh
py0c fact.py -o fact.qd # 編譯 fact.py 為 fact.qd
qd0c fact.qd -o fact.ll # 轉換 fact.qd 為 fact.ll

# 如果要在本機上跑 fact.ll ，此時可以用 clang fact.ll -o fact.o 然後執行 ./fact.o 就可以了。
# 但如果要用 RISCV 虛擬機上跑，則需要做下列動作 

ll0c fact.ll qd0lib.o -o fact.o  # 將 fact.ll 轉換為 RISC-V 上的目的檔（連結 qd0lib.o)
rv0vm fact.o  # RISC-V 虛擬機 rv0vm 執行 fact.o 
```

