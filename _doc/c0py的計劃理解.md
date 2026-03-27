# c0py 的計劃理解

## 專案概述

c0py 是一個自製電腦工具鏈專案，用 C + Python 打造簡易電腦系統，包含：
- **編譯器** (C 語言)
- **解譯器** (Python)
- **作業系統** (xv7)

## 工具流程

### C 語言流程 (.c → RISC-V)
```
.c 檔案 → (c0c) → .ll → (ll0c) → .s → (rv0as) → .o → (rv0vm) 執行
```

1. **c0c**: c0 編譯器，將簡化版 C 編譯為 LLVM IR (.ll)
2. **ll0c**: LLVM IR 組譯器，將 .ll 轉換為 RISC-V 組語 (.s)
3. **rv0as**: RISC-V 組譯器，將 .s 組裝為目的檔 (.o)
4. **rv0vm**: RISC-V 虛擬機，執行 .o 檔案

### Python 語言流程 (.py → host 或 RISC-V)
```
.py → (py0c) → .qd → (qd0c) → .ll → (ll0c) → .s → (rv0as) → .o → (rv0vm)
```

1. **py0c**: py0 編譯器，將簡化版 Python 編譯為四元組 (.qd)
2. **qd0c**: qd0 編譯器，將四元組轉換為 LLVM IR (.ll)

## 測試驗證

### C 語言測試 (test_all.sh)
- 執行 `test_all.sh --all` 會測試所有 `_data/*.c` 檔案
- 測試流程：編譯 → 組譯 → 執行 → 驗證輸出
- 預期結果從 `// expected:` 註解讀取
- 目前 **25 個測試全部通過**

測試範例：
- `fact.c`: 遞迴計算 5! = 120
- `test_arith.c`: 算術運算 (10+5)*2/3+20 = 30
- `test_struct.c`: 結構體測試
- `test_float.c`: 浮點數測試

### 作業系統測試 (xv7)
- **run.sh**: 啟動 xv7 系統 (make qemu)
- **net_test.sh**: 測試網路功能 (UDP Echo)
- **http_test.sh**: 測試 HTTP 伺服器
- **telnet_test.sh**: 測試 Telnet 伺服器
- **vim_test.sh**: 測試 Vim 編輯器

xv7 基於 xv6-riscv，已支援網路功能 (tcpip0)。

## 目錄結構

```
c0py/
├── c0/           # c0 編譯器 (c0c)
├── py0/          # py0 編譯器 (py0c) 和直譯器 (py0i)
├── qd0/          # 四元組 VM (qd0c, qd0lib)
├── ll0/          # LLVM IR 工具 (ll0i, ll0c)
├── rv0/          # RISC-V 工具鏈 (rv0as, rv0vm, rv0objdump)
├── xv7/          # 作業系統 (基於 xv6-riscv)
├── _data/        # 測試用的 .c 和 .py 檔案
├── test.sh       # 執行 test_all.sh --all
├── test_all.sh   # 自動測試所有 C 程式
└── Makefile      # 自動化建置和測試
```

## 已實現功能

1. **c0c**: 可編譯多種 C 程式（算術、邏輯、函式、結構體、指標、陣列、switch、union 等）
2. **ll0c**: 將 LLVM IR 編譯為 RISC-V 組語
3. **rv0as**: 組譯 RISC-V 目的檔
4. **rv0vm**: RISC-V 虛擬機，可執行編譯後的程式
5. **xv7**: 作業系統，已具備網路功能