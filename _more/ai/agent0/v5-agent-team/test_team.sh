#!/bin/bash
# test_team.sh - Agent0Team 自動化測試腳本

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# 顏色輸出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# 測試模式
MODE="${1:-all}"

log_section() {
    echo ""
    echo "══════════════════════════════════════"
    echo " $1"
    echo "══════════════════════════════════════"
}

check_dependencies() {
    log_section "檢查依賴"
    
    if ! command -v python3 &> /dev/null; then
        echo -e "${RED}[ERROR]${NC} Python3 未安裝"
        exit 1
    fi
    
    if ! python3 -c "import aiohttp" &> /dev/null 2>&1; then
        echo -e "${RED}[ERROR]${NC} aiohttp 未安裝，請執行：pip install aiohttp"
        exit 1
    fi
    
    echo -e "${GREEN}[OK]${NC} 依賴檢查完成"
}

run_python_tests() {
    log_section "執行 Python 測試"
    
    case $MODE in
        unit)
            python3 test_team.py unit
            ;;
        integration)
            python3 test_team.py integration
            ;;
        smoke)
            python3 test_team.py smoke
            ;;
        all)
            python3 test_team.py all
            ;;
        *)
            echo "未知模式: $MODE"
            echo "使用方式: $0 [unit|integration|smoke|all]"
            exit 1
            ;;
    esac
}

main() {
    echo "══════════════════════════════════════"
    echo " Agent0Team 自動化測試"
    echo "══════════════════════════════════════"
    echo ""
    echo "測試模式: $MODE"
    echo ""
    
    check_dependencies
    
    run_python_tests
    
    log_section "測試完成"
    echo -e "${GREEN}所有測試通過！${NC}"
}

main