# OpenCode 工作記憶

## 專案背景

**c0py** - 自製電腦工具鏈，用 C + Python 打造簡易電腦系統

```
c0py/
├── c0/           # c0 編譯器 (c0c)
├── py0/          # py0 編譯器
├── qd0/          # 四元組 VM
├── ll0/          # LLVM IR 工具
├── rv0/          # RISC-V 工具鏈 (rv0as, rv0vm, rv0objdump)
├── xv7/          # 作業系統 (基於 xv6-riscv)
├── _data/        # 測試用的 .c 檔案 (25個測試)
└── _doc/         # 文件
```

## 目前進度

### Phase 0: xv7 POSIX 相容性改造 (準備開始)

**目標**: 將 xv7 的 syscall 改為 Linux RISC-V 64-bit 標準

**需要修改的檔案**:
- `xv7/kernel/syscall.h` - syscall 編號
- `xv7/kernel/syscall.c` - syscall 表 + ioctl

**編號對照表**:

| 現有 | 改為 | 名稱 |
|------|------|------|
| 1 | 220 | clone |
| 2 | 93 | exit |
| 3 | 260 | wait4 |
| 5 | 0 | read |
| 16 | 1 | write |
| 15 | 2 | open |
| 21 | 3 | close |
| 8 | 5 | fstat |
| 12 | 12 | brk |
| 11 | 172 | getpid |
| 32 | 16 | ioctl (consolemode) |

**注意**:
- vim.c 使用 `consolemode()`，需改用 ioctl(16) 實現
- xv7 的 `sys_consolemode` 保留但改用 ioctl

## 環境資訊

**工具位置**: `/opt/homebrew/bin/`
- riscv64-unknown-elf-gcc
- qemu-system-riscv64
- make, clang

**測試命令**:
```bash
# 現有測試
cd c0py && ./test_all.sh --all

# xv7 測試
cd xv7 && make qemu
```

## 計劃文檔

- `_doc/c0py-改進計劃.md` - 完整改進計劃
- `_doc/c0py的計劃理解.md` - 專案理解

## 接下來的工作

1. 修改 `xv7/kernel/syscall.h` (已讀取內容)
2. 修改 `xv7/kernel/syscall.c` 
3. 測試編譯 `cd xv7 && make qemu`
4. 繼續 Phase 1: rv0vm 改進

---

## 檔案位置摘要

| 檔案 | 說明 |
|------|------|
| `rv0/rv0vm.c` | RISC-V 虛擬機 (1MB RAM, 支援部分指令) |
| `rv0/rv0as.c` | RISC-V 組譯器 |
| `xv7/kernel/syscall.h` | **待修改** - syscall 編號 |
| `xv7/kernel/syscall.c` | **待修改** - syscall 表 |
| `xv7/user/vim.c` | vim 程式 (依賴 consolemode) |
| `xv7/kernel/console.c` | consolemode() 實作位置 |