# 混合策略實作計劃：AST + LLM

## 設計目標

結합 AST 翻譯的確定性與 LLM 的廣泛語意理解能力，實現高效且廣泛的 Python to Rust 轉換。

## 系統架構

```
┌─────────────────────────────────────────────────────────────┐
│                      py2rs Hybrid                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │   Input     │───▶│   AST       │───▶│   LLM       │     │
│  │  Python     │    │   Parser    │    │   Enhancer  │     │
│  │  Code       │    │             │    │             │     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
│                            │                   │           │
│                            ▼                   ▼           │
│                     ┌─────────────┐    ┌─────────────┐     │
│                     │   AST       │    │   Rust      │     │
│                     │   Translator │    │   Generator │     │
│                     │  (基礎結構)   │    │ (複雜語法)   │     │
│                     └─────────────┘    └─────────────┘     │
│                            │                   │           │
│                            └─────────┬─────────┘           │
│                                      ▼                     │
│                            ┌─────────────┐               │
│                            │   Output    │               │
│                            │   Rust Code │               │
│                            └─────────────┘               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 實作階段

### 階段一：基礎設施建設

#### 1. 模組化重構現有程式碼

```
py2rs/
├── ast_parser.py      # Python AST 解析
├── ast_translator.py # AST 到 Rust 基礎翻譯
├── llm_enhancer.py    # LLM 增強模組
├── generator.py       # 最終 Rust 程式生成
├── config.py          # 配置管理
└── main.py           # 主程式入口
```

#### 2. 建立 AST 翻譯範圍

明確定義 AST 負責的範圍：

| 類別 | 支援 |
|------|------|
| 函式定義 | `def foo(x: int) -> int` |
| 基本型別 | `int`, `float`, `bool`, `str` |
| 控制流 | `if/elif/else`, `while`, `for` |
| 運算 | `+`, `-`, `*`, `/`, `%` |
| 比較 | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| 邏輯 | `and`, `or`, `not` |
| 賦值 | `=`, `+=`, `-=` |
| 回傳 | `return` |
| 簡單呼叫 | 內建函式呼叫 |

### 階段二：LLM 整合

#### 1. LLM API 整合

```python
# llm_enhancer.py
import os
import json

class LLMEnhancer:
    def __init__(self, provider="openai"):
        self.provider = provider
        self.api_key = os.getenv("OPENAI_API_KEY")
    
    def enhance(self, partial_rust_code: str, python_context: str) -> str:
        """增強翻譯結果"""
        prompt = f"""
        下面是部分翻譯的 Rust 程式碼，還有一些無法翻譯的 Python 語法。
        請完成翻譯並修正可能的錯誤。

        Python 上下文：
        {python_context}

        現有 Rust 程式碼：
        {partial_rust_code}

        請輸出完整的 Rust 程式碼。
        """
        # 調用 LLM API
        return self._call_llm(prompt)
```

#### 2. 處理無法翻譯的語法

當遇到這些情況時，交由 LLM 處理：

- `import` 第三方模組
- list/dict 複雜操作
- lambda 表達式
- 元組解包
- 特殊屬性存取
- 反射機制

#### 3. 設計回退機制

```python
def translate_node(node, context):
    if can_translate_with_ast(node):
        return ast_translate(node, context)
    else:
        # 交由 LLM 處理
        return llm_enhancer.translate(node, context)
```

### 階段三：優化與整合

#### 1. 錯誤修正

利用 LLM 修正 AST 翻譯的可能錯誤：

```python
def verify_and_fix(rust_code: str) -> str:
    """驗證並修正 Rust 程式碼"""
    issues = check_syntax(rust_code)
    if issues:
        return llm_enhancer.fix(issues, rust_code)
    return rust_code
```

#### 2. 最佳化建議

讓 LLM 提供 Rust 慣用語建議：

```python
def optimize(rust_code: str) -> str:
    """將 Python 風格轉為 Rust 風格"""
    return llm_enhancer.optimize(rust_code)
```

#### 3. 依賴處理

自動識別並添加 Rust 依賴：

```python
def handle_dependencies(python_code: str) -> list:
    """識別 Python import 對應的 Rust crate"""
    imports = extract_imports(python_code)
    return map_python_imports_to_rust_crates(imports)
```

## 實作細節

### 1. 配置管理

```python
# config.py
class Config:
    AST_ONLY_MODE = False  # 僅使用 AST 翻譯
    LLM_ENABLED = True     # 啟用 LLM 增強
    LLM_PROVIDER = "openai" # LLM 提供者
    MODEL = "gpt-4"        # 使用模型
    
    # LLM 回退策略
    FALLBACK_ON_ERROR = True
    MAX_RETRIES = 3
```

### 2. 日誌與除錯

```python
def translate_with_logging(python_code: str):
    """翻譯並記錄詳細日誌"""
    print(f"[AST] 解析 {len(python_code)} 行...")
    
    ast_result = ast_translate(python_code)
    print(f"[AST] 完成，翻譯了 {ast_result.stats.nodes} 個節點")
    print(f"[AST] 未處理: {ast_result.unhandled}")
    
    if ast_result.unhandled:
        print(f"[LLM] 處理 {len(ast_result.unhandled)} 個未處理節點...")
        final_result = llm_enhancer.complete(ast_result, python_code)
    else:
        final_result = ast_result
    
    return final_result
```

### 3. 測試策略

```bash
# 測試矩陣
test_cases/
├── simple/           # 純 AST 可處理
│   ├── factorial.py
│   ├── fib.py
│   └── loop.py
├── moderate/         # 需要 LLM
│   ├── with_import.py
│   ├── complex_list.py
│   └── dict_operations.py
└── complex/          # 複雜場景
    ├── full_app.py
    └── asyncio_example.py
```

## 使用流程

```bash
# 基本用法（自動混合）
python py2rs.py input.py -o output.rs

# 僅使用 AST（快速但不支援複雜語法）
python py2rs.py input.py -o output.rs --ast-only

# 指定 LLM 提供者
python py2rs.py input.py -o output.rs --llm openai --model gpt-4

# 顯示詳細日誌
python py2rs.py input.py -o output.rs -v
```

## 預期成果

| 指標 | 目標 |
|------|------|
| 簡單程式覆蓋率 | 95%+ |
| 複雜程式覆蓋率 | 70%+ |
| 翻譯正確率 | 90%+ |
| 效能 | < 5 秒/檔 |

## 風險與對策

| 風險 | 對策 |
|------|------|
| LLM API 不穩定 | 實作重試機制與本地回退 |
| API 費用過高 | 快取結果、批次處理 |
| 翻譯不一致 | 固定 seed 或使用結構化輸出 |
| 隱私疑慮 | 提供本地 LLM 選項（如 llama.cpp） |

## 結論

這個混合策略能在保持確定的基礎上，利用 LLM 突破語法限制，實現更廣泛的 Python to Rust 轉換能力。