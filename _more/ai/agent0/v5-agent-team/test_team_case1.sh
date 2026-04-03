#!/bin/bash
# test_team_case1.sh - Agent0Team 測試案例 1
# Test: Planner x 1, Generator x 3, Evaluator x 3

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "══════════════════════════════════════"
echo " Agent0Team 測試案例 1"
echo " Planner x 1, Generator x 3, Evaluator x 3"
echo "══════════════════════════════════════"

python3 test_team_case1.py

echo "Test passed!"