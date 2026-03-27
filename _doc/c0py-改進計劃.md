# c0py 改進計劃

## 目標概述

將 c0py 工具鏈擴充為可執行 xv7 作業系統的虛擬機系統，並支援編譯器自我編譯。

| 組件 | 現況 | 目標 |
|------|------|------|
| xv7 | 基於 xv6-riscv，syscall 編號與 Linux 不同 | 改為 POSIX 標準 |
| rv0vm | 僅 1MB RAM，支援部分指令 | 可執行 xv7 (Host + Guest 模式) |
| c0c | 可編譯基本 C 程式 | 可編譯 xv7 + 自我編譯 |

---

## Phase 0: xv7 POSIX 相容性改造

**目標**: 將 xv7 的 syscall 改為 Linux RISC-V 64-bit 標準，與 rv0vm Host 模式一致

### 修改項目

1. **syscall.h** - 更新 syscall 編號

| 現有編號 | xv7 名稱 | Linux 標準 | 說明 |
|---------|----------|-----------|------|
| 1 | fork | 220 (clone) | 建立新程序 |
| 2 | exit | 93 | 結束程式 |
| 3 | wait | 260 (wait4) | 等待程序 |
| 5 | read | 0 | 讀取檔案 |
| 16 | write | 1 | 寫入檔案 |
| 15 | open | 2 | 開啟檔案 |
| 21 | close | 3 | 關閉檔案 |
| 8 | fstat | 5 | 檔案狀態 |
| 12 | sbrk | 12 (brk) | 記憶體配置 |
| 11 | getpid | 172 | 程序 ID |
| 32 | consolemode | 16 (ioctl) | 終端機控制 |

2. **syscall.c** - 更新 syscall 表，新增缺失的 syscall (brk, mmap 等)

3. **ioctl 實作** - 支援終端機控制 (TIOCSETA/TIOCGETA)，vim 需要

4. **vim.c** - 保持不動，consolemode() 改為內部使用 ioctl

### 驗證
```bash
cd xv7 && make qemu  # 確認正常運作
```

---

## Phase 1: rv0vm 改進 - Host 模式

**目標**: 在 host（iMac）上直接執行 C 程式，不需要 xv7

### 1.1 記憶體擴充
- 從 1MB 擴充至 128MB
- 實作安全的虛擬記憶體管理

### 1.2 新增指令支援
| 指令 | 說明 |
|------|------|
| fence, fence.i | 記憶體同步 |
| sfence.vma | TLB 清除 |
| csrrw/csrrs/csrrc | CSR 存取 |
| sret, mret | 異常返回 |

### 1.3 Linux/POSIX Syscall

Host 模式下將 RISC-V syscall 轉發至 host:

| syscall | 編號 | 說明 |
|--------|------|------|
| read | 0 | 讀取 |
| write | 1 | 寫入 |
| open | 2 | 開檔 |
| close | 3 | 關檔 |
| exit | 93 | 結束 |
| getpid | 172 | 程序 ID |
| brk | 12 | 記憶體 |
| clone | 220 | fork |

### 1.4 ELF 載入器
- 支援標準 ELF 格式
- 處理 relocation

### 執行範例
```bash
# 使用 c0py 工具鏈
c0c hello.c -o hello.ll
ll0c hello.ll -o hello.s
rv0as hello.s -o hello.o
rv0vm -m host hello.o
```

---

## Phase 2: rv0vm 改進 - Guest 模式

**目標**: 可執行 xv7 作業系統

### 2.1 記憶體映射 (Guest)

```
0x00000000 - 0x07FFFFFF   RAM (128MB)
0x10000000 - 0x10000FFF   UART0
0x40000000 - 0x4FFFFFFF   PLIC
0x80000000 - 0x80001FFF   VirtIO Block
0xFFFFFFF0                CLINT
```

### 2.2 裝置模擬

| 裝置 | 位址 | 功能 |
|------|------|------|
| UART | 0x10000000 | 序列輸出輸入 |
| PLIC | 0x40000000 | 中斷控制 |
| CLINT | 0xFFFFFFF0 | 計時器中斷 |
| VirtIO | 0x80000000 | 區塊裝置 |

### 2.3 Trap/Exception 處理

- Page fault (load/store/instruction)
- Timer interrupt
- External interrupt (UART, VirtIO)
- System call (ecall)

### 2.4 CSR 暫存器

| CSR | 名稱 | 說明 |
|-----|------|------|
| mstatus | Machine Status | 中斷狀態 |
| mie | Machine Interrupt Enable | 中斷致能 |
| mip | Machine Interrupt Pending | 待處理中斷 |
| mtvec | Machine Trap Vector | 例外向量 |
| mepc | Machine Exception PC | 例外位置 |
| mcause | Machine Cause | 例外原因 |
| sstatus | Supervisor Status | 監督模式狀態 |
| satp | Supervisor Address Translation | 分頁表 |

### 執行範例
```bash
# 編譯 xv7
cd xv7 && make

# 在 rv0vm 上執行
rv0vm -m guest -k kernel/kernel
```

---

## Phase 3: c0c 編譯器改進

**目標**: 可編譯 xv7，並能自我編譯

### 3.1 c0c 功能擴充

| 功能 | 說明 |
|------|------|
| 完整 C 語法 | struct, union, pointer, array |
| 多檔案編譯 | 支援 .c 和 .h |
| 標頭檔 include | #include |
| 更佳最佳化 | 產生更高效的程式碼 |

### 3.2 編譯 xv7

```bash
# 使用 c0py 編譯 xv7 kernel
c0c xv7/kernel/main.c -o main.ll
c0c xv7/kernel/proc.c -o proc.ll
# ... 其他模組
ll0c *.ll -o kernel.s
rv0as kernel.s -o kernel.o
rv0vm -m guest -k kernel.o
```

### 3.3 自我編譯

```bash
# 使用 c0c 編譯 c0c 本身
c0c c0/c0c/*.c -o c0c_new.ll
# 驗證新編譯器可以編譯相同程式
```

---

## 實作時程 (預估)

| Phase | 工作內容 | 預估時間 |
|-------|----------|----------|
| Phase 0 | xv7 POSIX 改造 | 1 週 |
| Phase 1 | rv0vm Host 模式 | 2 週 |
| Phase 2 | rv0vm Guest 模式 | 3-4 週 |
| Phase 3 | c0c 改進 + 自我編譯 | 2-3 週 |

---

## 驗證測試

```bash
# 1. xv7 POSIX 改造測試
cd xv7 && make qemu

# 2. Host 模式測試
cd c0py && ./test_all.sh --all  # 現有 25 個測試
rv0vm -m host hello.o           # 新功能

# 3. Guest 模式測試
cd xv7 && make
rv0vm -m guest -k kernel/kernel

# 4. 編譯器自我編譯測試
c0c c0/c0c/main.c -o main.ll
c0c c0/c0c/parser.c -o parser.ll
```

---

## 預期成果

1. ✅ xv7 syscall 與 Linux/POSIX 標準一致
2. ✅ rv0vm 可在 host 模式執行 C 程式
3. ✅ rv0vm 可在 guest 模式執行 xv7
4. ✅ c0c 可編譯 xv7 kernel
5. ✅ c0c 可自我編譯
6. ✅ 向後相容 (現有 25 個測試通過)