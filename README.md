# c0py -- 用 C + Python 打造簡易電腦工業

> 包含：編譯器 (C) + 解譯器 (Python) + 作業系統

## 工具流程

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

然後我們會將 [xv6-riscv] 作業系統，修改為 [xv7]

## 語言與格式

* c0 -- 簡化後的 C 語言，副檔名為 .c
* py0 -- 簡化後的 Python 語言，副檔名為 .py
* ll0 -- 簡化後的 LLVM IR 文字格式，副檔名為 .ll
* qd0 -- 動態語言虛擬機，採用 quadruple 四元組格式，副檔名為 .qd
* rv0 -- RISCV 指令集簡化版

## 實作工具

[c0]:compiler/c0/
[py0]:compiler/py0/
[qd0]:compiler/qd0/
[ll0]:compiler/ll0/
[rv0]:compiler/rv0/

[c0c]:compiler/c0/c0c/
[py0c]:compiler/py0/py0c/
[py0i]:compiler/py0/py0i/
[qd0c]:compiler/qd0/qd0c/
[qd0lib]:compiler/qd0/qd0c/qd0lib.c
[ll0i]:compiler/ll0/ll0i/
[ll0c]:compiler/ll0/ll0c/
[rv0as.c]:compiler/rv0/rv0as.c
[rv0vm.c]:compiler/rv0/rv0vm.c
[rv0objdump.c]:compiler/rv0/rv0objdump.c

* compiler -- 編譯器
    * [x] [c0c] -- c0 之編譯器，類似 gcc (C 語言:ccc 用 AI 建構)
    * [x] [py0c] -- py0 之編譯器，輸出 qd 檔案 (C 語言:ccc 用 AI 建構)
    * [x] [py0i] -- py0 的解譯器，可以直接執行 .py 的檔案 (Python: ccc 用 AI 建構，但先用 [py0c] 編譯為 py0c.o ，交由 rv0vm 執行)
    * [x] [qd0c] -- qd0 轉為 ll0 的編譯器 (C 語言:ccc 用 AI 建構)
    * [x] [qd0lib] -- qd0 的指令呼叫與函式庫 (C 語言:ccc 用 AI 建構)
    * [x] [ll0i] -- ll0 中間碼虛擬機，類似 lli (C 語言:ccc 用 AI 建構)
    * [x] [ll0c] -- 簡化後的 LLVM IR 中間碼組譯器，類似 llc (C 語言:ccc 用 AI 建構)
    * [x] [rv0] 工具鏈 -- 包含 [rv0as.c], [rv0vm.c], [rv0objdump.c] (C語言:ccc 用 AI 建構)

[xv7]:os/xv7
[xv6-riscv]:https://github.com/mit-pdos/xv6-riscv

* os -- 作業系統
    * [x] [xv7] -- 擴充自 [xv6-riscv]，支援 [tcpip0] (C 語言:ccc 用 AI 建構， OpenCode+BigPickle) 

