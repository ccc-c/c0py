# QEMU RV32 限制：只有 Hart 0 能執行

## 問題描述

當使用 `qemu-system-riscv32` 搭配 `-bios none` 啟動時，只有 hart 0 會實際進入 S-mode 執行我們的 OS 代碼。其他 hart (1, 2, 3) 不會執行。

## 環境

- QEMU: riscv32
- 啟動選項: `-bios none`
- 核心數: `-smp 4`

## 觀察到的行為

### 使用 `-bios none` 時
- Hart 0: ✅ 正常執行 os_main
- Hart 1-3: ❌ 不會進入 S-mode，不執行我們的程式碼

### 使用 OpenSBI 時
- Hart 0-3: ✅ 全部都能執行

## 根本原因

這是 **QEMU 對 RV32 (32-bit) 實作的限制**，不是 RISC-V 規格書的問題。

根據 RISC-V 規格書，每個 hart 都應該能夠：
1. 從 ROM 啟動
2. 執行 M-mode 代碼
3. 進入 S-mode

但 QEMU 在 RV32 模式下沒有完整實現這個功能。

## 解決方案

### 方案 1：使用 OpenSBI
```bash
qemu-system-riscv32 -nographic -machine virt -kernel os.elf -smp 4
```
QEMU 內建的 OpenSBI 會正確啟動所有 hart。

### 方案 2：改用 64 位元
像 xv6 那樣使用 `qemu-system-riscv64`：
```bash
qemu-system-riscv64 -nographic -machine virt -kernel os.elf -smp 4
```

### 方案 3：接受限制
如果只需要 2 個 hart (hart 0 和 1)，可以繼續使用 `-bios none`。

## 測試驗證

```bash
# 測試腳本
qemu-system-riscv32 -nographic -machine virt -bios none -kernel os.elf -smp 4

# 輸出應該只會看到 Task0 和 Task1 的 "Created!"
```

## 參考資料

- xv6-riscv: 使用 `qemu-system-riscv64` + OpenSBI
- RISC-V SBI 規格書
- QEMU RISC-V 模擬器文件

## 相關檔案

- `start.s` - 啟動程式碼
- `os.c` - OS 初始化
- `task.c` - 排程器實作
