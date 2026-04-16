寫出完整的 c4 程式碼，要能自我編譯，先不用輸出 ELF 檔。

# C4 ByteCode ELF 格式

## 概述

C4 編譯器輸出的 ByteCode 以 ELF 格式包裝，由獨立的 `c4vm` 虛擬機執行。

```
┌──────────────────┐      ┌──────────────────┐
│   hello.elf      │      │   c4vm (native)   │
│  ┌────────────┐  │      │                   │
│  │ .text      │──┼──→   │   讀取 ELF        │
│  │ (ByteCode) │  │      │   執行 ByteCode   │
│  ├────────────┤  │      │   管理堆疊/記憶體  │
│  │ .data      │  │      │                   │
│  ├────────────┤  │      └───────────────────┘
│  │ .shstrtab  │  │
│  ├────────────┤  │
│  │ .symtab    │  │
│  ├────────────┤  │
│  │ .strtab    │  │
│  └────────────┘  │
└──────────────────┘
```

## ByteCode 指令集

**完全不需要修改**。現有 C4 VM 指令集 그대로 可用。

### 現有指令（與 vm.md 相同）

| 助記符 | 說明 |
|--------|------|
| LLA n | 載入本地位址 |
| IMM n | 立即值 |
| JMP n | 跳躍 |
| JSR n | 跳至副程式 |
| BZ n | 為零分支 |
| BNZ n | 非零分支 |
| ENT n | 進入函式 |
| ADJ n | 調整堆疊 |
| LEV | 離開函式 |
| LI/LC | 載入整數/字元 |
| SI/SC | 儲存整數/字元 |
| PSH | 推入堆疊 |
| 運算指令 | ADD/SUB/MUL/DIV/... |
| 浮點指令 | FADD/FSUB/FMUL/... |
| 系統呼叫 | PRTF/OPEN/READ/... |

## ELF Section 設計

| Section | 內容 |
|---------|------|
| `.text` | C4 ByteCode（VM 位碼） |
| `.data` | 全域變數初始值 |
| `.shstrtab` | Section 名稱字串表（`.text`, `.data`, `.shstrtab`, `.symtab`, `.strtab`） |
| `.symtab` | 符號表（函式名、全域變數） |
| `.strtab` | 符號名稱字串表 |

## 需要修改的資料表示

### 目前 C4 的問題

現在 C4 的指標是**程式內部的虛擬記憶體位址**，不符合 ELF 的檔案偏移：

| 目前 | ELF 格式需求 |
|------|--------------|
| `p = "char double..."` 是編譯器內部指標 | 需要 `.shstrtab` section，string table index |
| `ival` 直接存指標值 | 需要 `.data` section 的檔案偏移 |
| 無符號表 | 需要 `.symtab` + `.strtab` |
| `JSR main` 的 main 位址 | 需要 ELF symbol 的 value（偏移） |

### 修改要點

#### 1. 關鍵字表 → `.shstrtab`

```c
// 現在：直接指標
p = "char double else enum float for break continue if int long return short sizeof while struct";

// ELF：建立 .shstrtab section
char *shstrtab[] = { "", ".text", ".data", ".shstrtab", ".symtab", ".strtab" };
```

#### 2. 字串字面量 → `.data`

```c
// 現在：直接指標指向 data 區域
"hello"  →  ival = (long)pp;  // pp 是 data 區域指標

// ELF：輸出到 .data section，用偏移量代表
// .data[0] = 'h', .data[1] = 'e', ...
// ival = offset_of_string_in_data_section
```

#### 3. 符號表 → `.symtab`

```c
// 建立符號表結構
struct Elf64_Sym {
    long st_name;    // .strtab 中的偏移
    long st_info;    // Bind/Type
    long st_other;   // 0
    long st_shndx;   // section index
    long st_value;   // 符號位址（偏移）
    long st_size;    // 大小
};

// 每個函式/全域變數都是一個符號
```

#### 4. 位址表示

| 現在 | ELF |
|------|-----|
| `id->v_val = (long)(e + 1)` | `st_value = offset_of_bytecode_in_text_section` |
| `data = data + sizeof(long)` | `.data` section 的偏移遞增 |

## 位址模型

- ByteCode 使用**相對位移**（位置無關）
- `JMP n`、`JSR n` 中的 `n` 是相對於目前 pc 的偏移
- c4vm 將 `.text` 載入記憶體後，用指標 + 偏移執行

## c4vm 的職責

1. 解析 ELF 格式
2. 讀取 `.text` section（ByteCode）
3. 讀取 `.data` section（全域變數）
4. 從 `.symtab` 解析符號（找到 `main` 的 st_value）
5. 配置記憶體（堆疊、資料段）
6. 初始化全域變數
7. 執行 ByteCode（邏輯與現有 `run()` 相同）

## 範例

```
c4 hello.c -elf hello.elf   →  產生 hello.elf
c4vm hello.elf              →  由 c4vm 執行
```

## 結論

VM 指令集不需要任何改變，但需要修改：

1. **資料表示**：從指標改為 section 偏移
2. **新增 sections**：`.shstrtab`、`.symtab`、`.strtab`
3. **字串處理**：關鍵字表、字面量都要輸出到對應 section
4. **符號解析**：建立完整符號表供 c4vm 使用