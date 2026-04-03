#!/bin/bash

# test_hello.sh - 測試 hello.py 腳本

echo "=== 開始測試 hello.py ==="

# 執行 hello.py 並檢查輸出
python3 hello.py > output.txt 2>&1

if [ $? -eq 0 ]; then
    echo "✓ hello.py 執行成功"
    echo "=== 輸出內容 ==="
    cat output.txt
else
    echo "✗ hello.py 執行失敗"
    cat output.txt
    exit 1
fi

# 檢查輸出是否包含預期內容
if grep -q "Hello, World!" output.txt; then
    echo "✓ 輸出包含 'Hello, World!'"
else
    echo "✗ 輸出缺少 'Hello, World!'"
    exit 1
fi

echo "=== 所有測試通過 ==="
rm -f output.txt
