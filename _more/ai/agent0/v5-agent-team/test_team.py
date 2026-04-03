#!/usr/bin/env python3
# test_team.py - Agent0Team 測試脚本
# Run: python test_team.py [unit|integration|smoke|all]

import asyncio
import os
import sys
import shutil
import tempfile
import json
from datetime import datetime
from pathlib import Path

# 临时工作目录
TEST_WORKSPACE = None

def setup_test_env():
    """Setup test environment"""
    global TEST_WORKSPACE
    TEST_WORKSPACE = tempfile.mkdtemp(prefix="agent0team_test_")
    os.environ["HOME"] = TEST_WORKSPACE
    
    memory_dir = os.path.join(TEST_WORKSPACE, ".agent0", "team", "memory")
    shared_dir = os.path.join(TEST_WORKSPACE, ".agent0", "team", "shared")
    os.makedirs(memory_dir, exist_ok=True)
    os.makedirs(shared_dir, exist_ok=True)
    
    return TEST_WORKSPACE

def cleanup_test_env():
    """Cleanup test environment"""
    global TEST_WORKSPACE
    if TEST_WORKSPACE and os.path.exists(TEST_WORKSPACE):
        shutil.rmtree(TEST_WORKSPACE)

def test_memory_functions():
    """Test memory management functions"""
    print("═══ 測試記憶管理 ═══")
    
    from agent0team import (
        load_memory, save_memory, append_memory, get_memory,
        read_shared, write_shared, append_shared
    )
    
    os.makedirs(os.path.join(TEST_WORKSPACE, ".agent0", "team", "memory"), exist_ok=True)
    os.makedirs(os.path.join(TEST_WORKSPACE, ".agent0", "team", "shared"), exist_ok=True)
    
    # Test append_memory and load_memory
    append_memory("planner", 0, "test content 1", 1)
    items = load_memory("planner", 0)
    assert len(items) == 1, f"Expected 1, got {len(items)}"
    assert items[0]["content"] == "test content 1"
    print("✅ append_memory/load_memory")
    
    # Test get_memory
    mem = get_memory("planner", 0)
    assert len(mem) == 1
    assert mem[0] == "test content 1"
    print("✅ get_memory")
    
    # Test append more
    append_memory("planner", 0, "test content 2", 2)
    items = load_memory("planner", 0)
    assert len(items) == 2
    print("✅ append_memory multiple")
    
    # Test shared files
    write_shared("test.txt", "hello")
    content = read_shared("test.txt")
    assert content == "hello", f"Expected 'hello', got '{content}'"
    print("✅ write_shared/read_shared")
    
    append_shared("test.txt", "world")
    content = read_shared("test.txt")
    assert "world" in content
    print("✅ append_shared")
    
    # Test different agents
    append_memory("generator", 0, "gen output 1", 1)
    append_memory("generator", 1, "gen output 2", 1)
    append_memory("evaluator", 0, "eval result 1", 1)
    
    gen0_mem = get_memory("generator", 0)
    gen1_mem = get_memory("generator", 1)
    eval0_mem = get_memory("evaluator", 0)
    
    assert len(gen0_mem) == 1
    assert len(gen1_mem) == 1
    assert len(eval0_mem) == 1
    print("✅ multiple agents memory")
    
    print("✅ 記憶管理測試通過\n")

def test_shared_context():
    """Test shared context"""
    print("═══ 測試共享上下文 ═══")
    
    from agent0team import (
        conversation_history, key_info,
        build_context, get_shared_context
    )
    
    # Add some history
    conversation_history.append("  <user>test input</user>")
    conversation_history.append("  <assistant>test response</assistant>")
    
    # Add key info
    key_info.append("test key info")
    
    context = get_shared_context()
    assert "test input" in context
    assert "test key info" in context
    print("✅ get_shared_context")
    
    print("✅ 共享上下文測試通過\n")

def test_plan_task():
    """Test Planner plan_task"""
    print("═══ 測試 Planner ═══")
    
    from agent0team import (
        plan_task, current_iteration,
        build_context, append_memory
    )
    
    current_iteration = 0
    
    context = build_context()
    
    # Test simple plan
    plan = asyncio.run(plan_task("list files in current directory", context))
    
    assert plan is not None
    assert isinstance(plan.steps, list)
    print(f"✅ plan_task: {len(plan.steps)} steps")
    
    print("✅ Planner 測試通過\n")

def test_execute_step():
    """Test Generator execute_step"""
    print("═══ 測試 Generator ═══")
    
    from agent0team import execute_step, build_context
    
    context = build_context()
    
    # Test ls command
    step = "<shell>ls -la</shell>"
    result = asyncio.run(execute_step(step, 0, context))
    
    assert result["success"] == True
    assert "command" in result
    print("✅ execute_step: ls")
    
    # Test other commands
    step = "<shell>pwd</shell>"
    result = asyncio.run(execute_step(step, 1, context))
    assert result["success"] == True
    print("✅ execute_step: pwd")
    
    print("✅ Generator 測試通過\n")

def test_evaluate_output():
    """Test Evaluator evaluate_output"""
    print("═══ 測試 Evaluator ═══")
    
    from agent0team import (
        evaluate_output, plan_task,
        build_context, execute_plan, current_iteration
    )
    
    context = build_context()
    current_iteration = 0
    
    # First get a plan
    plan = asyncio.run(plan_task("list files", context))
    
    # Execute plan
    gen_output = asyncio.run(execute_plan(plan, context, 0))
    
    # Evaluate output
    result = asyncio.run(evaluate_output(
        "list files",
        [gen_output],
        plan,
        0
    ))
    
    assert result is not None
    assert "passed" in result
    assert "score" in result
    print(f"✅ evaluate_output: passed={result['passed']}, score={result['score']}")
    
    print("✅ Evaluator 測試通過\n")

def test_team_loop():
    """Test complete team loop"""
    print("═══ 測試團隊循環 ═══")
    
    from agent0team import run_team_loop, clear_shared
    
    clear_shared()
    
    response, log = asyncio.run(run_team_loop("list files in current directory"))
    
    assert response is not None
    assert len(log) > 0
    print(f"✅ team_loop: {len(log)} log entries")
    print(f"   response: {response[:100]}...")
    
    print("✅ 團隊循環測試通過\n")

def test_multiple_agents():
    """Test multiple generators/evaluators"""
    print("═══ 測試多 agent ═══")
    
    from agent0team import (
        NUM_GENERATORS, NUM_EVALUATORS,
        run_team_loop, clear_shared
    )
    
    clear_shared()
    
    # Temporarily change settings
    original_gen = NUM_GENERATORS
    original_eval = NUM_EVALUATORS
    
    # This will use default values
    response, log = asyncio.run(run_team_loop("list files"))
    
    assert response is not None
    print(f"✅ multiple_agents test")
    
    print("✅ 多 agent 測試通過\n")

def run_unit_tests():
    """Run unit tests"""
    print("╔══════════════════════════════════════╗")
    print("║        Agent0Team 單元測試         ║")
    print("╚══════════════════════════════════════╝\n")
    
    setup_test_env()
    
    try:
        test_memory_functions()
        test_shared_context()
    except Exception as e:
        print(f"❌ 測試失敗: {e}")
        import traceback
        traceback.print_exc()
        cleanup_test_env()
        sys.exit(1)
    
    cleanup_test_env()
    print("✅ 所有單元測試通過\n")

def run_integration_tests():
    """Run integration tests"""
    print("╔══════════════════════════════════════╗")
    print("║       Agent0Team 整合測試          ║")
    print("╚══════════════════════════════════════╝\n")
    
    setup_test_env()
    
    try:
        test_plan_task()
        test_execute_step()
        test_evaluate_output()
    except Exception as e:
        print(f"❌ 測試失敗: {e}")
        import traceback
        traceback.print_exc()
        cleanup_test_env()
        sys.exit(1)
    
    cleanup_test_env()
    print("✅ 所有整合測試通過\n")

def run_smoke_tests():
    """Run smoke tests"""
    print("╔══════════════════════════════════════╗")
    print("║        Agent0Team 冒煙測試         ║")
    print("╚══════════════════════════════════════╝\n")
    
    setup_test_env()
    
    try:
        test_team_loop()
    except Exception as e:
        print(f"❌ 測試失敗: {e}")
        import traceback
        traceback.print_exc()
        cleanup_test_env()
        sys.exit(1)
    
    cleanup_test_env()
    print("✅ 冒煙測試通過\n")

def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "all"
    
    print(f"測試模式: {mode}\n")
    
    if mode == "unit":
        run_unit_tests()
    elif mode == "integration":
        run_integration_tests()
    elif mode == "smoke":
        run_smoke_tests()
    elif mode == "all":
        setup_test_env()
        
        try:
            test_memory_functions()
            test_shared_context()
            test_plan_task()
            test_execute_step()
            test_evaluate_output()
            test_team_loop()
        except Exception as e:
            print(f"❌ 測試失敗: {e}")
            import traceback
            traceback.print_exc()
            cleanup_test_env()
            sys.exit(1)
        
        cleanup_test_env()
        print("╔══════════════════════════════════════╗")
        print("║         所有測試通過！              ║")
        print("╚══════════════════════════════════════╝\n")
    else:
        print(f"未知模式: {mode}")
        print("使用方式: python test_team.py [unit|integration|smoke|all]")
        sys.exit(1)

if __name__ == "__main__":
    main()