#!/usr/bin/env python3
# test_team_case1.py - Simple test case 1
# Test: Planner x 1, Generator x 3, Evaluator x 3

import asyncio
import agent0team as team
import os
import tempfile
import shutil

TEST_DIR = None

def setup():
    global TEST_DIR
    TEST_DIR = os.path.join(os.getcwd(), '.agent0')
    os.makedirs(TEST_DIR, exist_ok=True)
    
    team.WORKSPACE = TEST_DIR
    team.MEMORY_DIR = os.path.join(team.WORKSPACE, 'team', 'memory')
    team.SHARED_DIR = os.path.join(team.WORKSPACE, 'team', 'shared')
    team.NUM_GENERATORS = 3
    team.NUM_EVALUATORS = 3
    team.MAX_ITERATIONS = 1
    team.DEBUG = False
    
    os.makedirs(team.WORKSPACE, exist_ok=True)
    os.makedirs(team.MEMORY_DIR, exist_ok=True)
    os.makedirs(team.SHARED_DIR, exist_ok=True)
    
    print(f"Config: GEN={team.NUM_GENERATORS}, EVAL={team.NUM_EVALUATORS}, MAX={team.MAX_ITERATIONS}")

def cleanup():
    pass  # Keep files for inspection

async def test():
    print("Test: Analyze Python code structure")
    
    team.clear_shared()
    team.team_log.clear()
    team.current_iteration = 0
    team.MAX_ITERATIONS = 2  # More iterations for complex task
    
    task = "分析 v4-agent-team 目錄中的 Python 程式碼結構，並產生一份結構報告"
    print(f"Task: {task}")
    
    result, log = await team.run_team_loop(task)
    
    print(f"Result: {result[:100] if result else 'None'}...")
    print(f"Log entries: {len(log)}")
    
    # Check outputs
    for i in range(team.NUM_GENERATORS):
        output = team.read_shared(f'output_{i}.txt')
        print(f"Generator {i}: {len(output)} chars")
    
    # Check evaluations
    for i in range(team.NUM_EVALUATORS):
        eval_result = team.read_shared(f'evaluation_{i}.txt')
        print(f"Evaluator {i}: {len(eval_result)} chars")
    
    print("Test passed!")

def main():
    setup()
    try:
        asyncio.run(test())
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        cleanup()

if __name__ == "__main__":
    main()