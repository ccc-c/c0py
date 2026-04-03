#!/usr/bin/env python3
# agent0team.py - Team AI Agent with Planner/Generator/Evaluator
# Run: python agent0team.py [--generators N] [--evaluators N] [--max-iterations N] [--debug]

import subprocess
import os
import sys
import asyncio
import aiohttp
import re
import json
import argparse
import tempfile
import shutil
from datetime import datetime
from pathlib import Path
from dataclasses import dataclass, field
from typing import Optional

import agent0

# ─── Configuration ───

WORKSPACE = os.path.expanduser("~/.agent0")
MODEL = "minimax-m2.5:cloud"
REVIEWER_MODEL = "minimax-m2.5:cloud"
MAX_TURNS = 5

NUM_GENERATORS = 1
NUM_EVALUATORS = 1
MAX_ITERATIONS = 3

DEBUG = False

# ─── Directories ───

MEMORY_DIR = os.path.join(WORKSPACE, "team", "memory")
SHARED_DIR = os.path.join(WORKSPACE, "team", "shared")

# ─── Global State ───

conversation_history = []
key_info = []
team_log = []
current_iteration = 0
current_task_id = None

# ─── Shared Agent0 instance for tool execution ───

shared_agent = None

def get_agent() -> agent0.Agent0:
    """Get shared Agent0 instance"""
    global shared_agent
    if shared_agent is None:
        shared_agent = agent0.Agent0(
            workspace=WORKSPACE,
            model=MODEL,
            reviewer_model=REVIEWER_MODEL,
            max_turns=MAX_TURNS
        )
    return shared_agent

# ─── Data Classes ───

@dataclass
class Plan:
    analysis: str = ""
    steps: list = field(default_factory=list)
    decision: str = "continue"
    feedback: str = ""

@dataclass
class TaskContext:
    user_input: str = ""
    current_plan: Optional[Plan] = None
    generators_outputs: list = field(default_factory=list)
    evaluators_results: list = field(default_factory=list)
    iteration: int = 0
    team_context: str = ""

# ─── Ollama API ───

async def call_ollama(prompt: str, system: str = "", model: str = MODEL) -> str:
    """Call Ollama API"""
    full_prompt = f"{system}\n\n{prompt}" if system else prompt
    
    payload = {
        "model": model,
        "prompt": full_prompt,
        "stream": False
    }
    
    async with aiohttp.ClientSession() as session:
        async with session.post(
            "http://localhost:11434/api/generate",
            json=payload,
            timeout=aiohttp.ClientTimeout(total=120)
        ) as resp:
            result = await resp.json()
            return result.get("response", "").strip()

# ─── Memory Management ───

def load_memory(agent_type: str, agent_id: int) -> list:
    """Load agent memory from JSON file"""
    filepath = os.path.join(MEMORY_DIR, f"{agent_type}_{agent_id}.json")
    try:
        with open(filepath, 'r') as f:
            data = json.load(f)
            return data.get("items", [])
    except (FileNotFoundError, json.JSONDecodeError):
        return []

def save_memory(agent_type: str, agent_id: int, items: list):
    """Save agent memory to JSON file"""
    os.makedirs(MEMORY_DIR, exist_ok=True)
    filepath = os.path.join(MEMORY_DIR, f"{agent_type}_{agent_id}.json")
    
    data = {
        "agent_type": agent_type,
        "agent_id": agent_id,
        "updated_at": datetime.now().isoformat(),
        "items": items
    }
    
    with open(filepath, 'w') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

def append_memory(agent_type: str, agent_id: int, content: str, iteration: int):
    """Append memory to agent"""
    items = load_memory(agent_type, agent_id)
    items.append({
        "timestamp": datetime.now().isoformat(),
        "content": content,
        "iteration": iteration
    })
    save_memory(agent_type, agent_id, items)

def get_memory(agent_type: str, agent_id: int, limit: int = 10) -> list:
    """Get agent memory (simplified)"""
    items = load_memory(agent_type, agent_id)
    return [item["content"] for item in items[-limit:]]

# ─── Shared File Management ───

def read_shared(filename: str) -> str:
    """Read shared file"""
    filepath = os.path.join(SHARED_DIR, filename)
    try:
        with open(filepath, 'r') as f:
            return f.read()
    except FileNotFoundError:
        return ""

def write_shared(filename: str, content: str):
    """Write shared file"""
    os.makedirs(SHARED_DIR, exist_ok=True)
    filepath = os.path.join(SHARED_DIR, filename)
    with open(filepath, 'w') as f:
        f.write(content)

def append_shared(filename: str, line: str):
    """Append line to shared file"""
    os.makedirs(SHARED_DIR, exist_ok=True)
    filepath = os.path.join(SHARED_DIR, filename)
    with open(filepath, 'a') as f:
        f.write(line + "\n")

def get_shared_context() -> str:
    """Get shared context for all agents"""
    context_parts = []
    
    if conversation_history:
        context_parts.append("<history>\n" + "\n".join(conversation_history[-MAX_TURNS*2:]) + "\n</history>")
    
    if key_info:
        items_xml = "\n".join(f"  <item>{k}</item>" for k in key_info)
        context_parts.append(f"<memory>\n{items_xml}\n</memory>")
    
    return "\n\n".join(context_parts)

def clear_shared():
    """Clear shared files"""
    os.makedirs(SHARED_DIR, exist_ok=True)
    for f in os.listdir(SHARED_DIR):
        if f.startswith("teamlog") or f.startswith("output") or f.startswith("evaluation"):
            os.remove(os.path.join(SHARED_DIR, f))

# ─── Context Building ───

def build_context():
    """Build context for LLM"""
    return get_shared_context()

def update_memory(user_input: str, assistant_response: str, tool_result: str = None):
    """Update conversation history"""
    conversation_history.append(f"  <user>{user_input}</user>")
    conversation_history.append(f"  <assistant>{assistant_response}</assistant>")
    if tool_result:
        conversation_history.append(f"  <tool>{tool_result[:500]}</tool>")
    
    while len(conversation_history) > MAX_TURNS * 4:
        conversation_history.pop(0)

async def extract_key_info(user_input: str, assistant_response: str):
    """Extract key info for long-term memory"""
    extract_prompt = f"""根據這段對話，有沒有需要長期記憶的關鍵資訊？
如果有，用以下格式輸出（最多 2 項）。如果沒有，輸出 <memory></memory>。

<memory>
  <item>要記憶的資訊 1</item>
  <item>要記憶的資訊 2</item>
</memory>

對話：
<user>{user_input}</user>
<assistant>{assistant_response}</assistant>"""
    
    try:
        result = await call_ollama(extract_prompt, "")
        matches = re.findall(r'<item>(.*?)</item>', result, re.DOTALL)
        for item in matches:
            item = item.strip()
            if item and item not in key_info:
                key_info.append(item)
    except:
        pass

# ─── Planner ───

PLANNER_SYSTEM = """你是 Planner，團隊的規劃���。

職責：
1. 分析用戶輸入，理解任務意圖
2. 制定執行計劃（包含具體的 shell 命令）
3. 匯總並檢視所有 Evaluator 的評估結果
4. 決定下一輪 plan（任務已完成 / 繼續優化 / 放棄任務）

重要規則：
- 用 <plan> 標籤輸出你的計劃
- 用 <analysis> 標籤輸出任務分析
- 用 <steps> 標籤輸出執行步驟（每個步驟必須用 <shell> 標籤包住命令）
- 用 <decision> 標籤輸出你的決定
- 任務已完成用 <end/> 結束
- 放棄任務用 <abandon/> 並說明原因"""

async def plan_task(user_input: str, context: str, feedback: str = None) -> Plan:
    """Plan task analysis and execution plan"""
    feedback_section = f"\n\n<feedback>{feedback}</feedback>" if feedback else ""
    
    prompt = f"""<context>{context}</context>

<user>{user_input}</user>{feedback_section}

請分析任務並制定執行計劃。每個步驟必須用 <shell> 標籤包住實際要執行的命令："""

    response = await call_ollama(prompt, PLANNER_SYSTEM)
    
    if DEBUG:
        print(f"[DEBUG] Planner response: {response[:300]}")
    
    plan = Plan()
    
    analysis_match = re.search(r'<analysis>(.*?)</analysis>', response, re.DOTALL)
    if analysis_match:
        plan.analysis = analysis_match.group(1).strip()
    
    step_matches = re.findall(r'<step>(.*?)</step>', response, re.DOTALL)
    plan.steps = [s.strip() for s in step_matches]
    
    if not plan.steps:
        shell_matches = re.findall(r'<shell>(.*?)</shell>', response, re.DOTALL)
        plan.steps = [f"<shell>{s.strip()}</shell>" for s in shell_matches]
    
    if not plan.steps:
        plan.steps = ["<shell>echo 'No command generated'</shell>"]
    
    decision_match = re.search(r'<decision>(.*?)</decision>', response, re.DOTALL)
    if decision_match:
        plan.decision = decision_match.group(1).strip()
    
    append_memory("planner", 0, f"plan: {plan.analysis[:100]}", current_iteration)
    
    return plan

async def review_evaluations(evaluations: list, plan: Plan) -> dict:
    """Review and汇总 all Evaluator results"""
    all_passed = all(e.get("passed", False) for e in evaluations)
    all_results = []
    
    for e in evaluations:
        all_results.append({
            "evaluator_id": e.get("evaluator_id", 0),
            "passed": e.get("passed", False),
            "score": e.get("score", 0),
            "feedback": e.get("feedback", "")
        })
    
    issues = []
    for e in evaluations:
        issues.extend(e.get("issues", []))
    
    consensus = "PASS" if all_passed else "FAIL"
    
    if all_passed:
        plan.decision = "complete"
    
    return {
        "overall": consensus,
        "all_passed": all_passed,
        "results": all_results,
        "issues": issues
    }

async def decide_next_plan(review_result: dict, plan: Plan, iteration: int) -> tuple[str, Plan]:
    """Decide next plan based on evaluation results"""
    if review_result["all_passed"]:
        return "complete", plan
    
    if iteration >= MAX_ITERATIONS - 1:
        append_memory("planner", 0, f"abandon: max iterations reached", iteration)
        return "abandon", plan
    
    feedback = "\n".join(review_result["issues"])
    plan.feedback = feedback
    
    prompt = f"""根據以下評估反饋，請制定改進計劃：

<previous_plan>
  analysis: {plan.analysis}
  steps: {plan.steps}
</previous_plan>

<feedback>{feedback}</feedback>

請用以下格式輸出改進計劃（每個步驟必須用 <shell> 標籤包住命令）：
<plan>
  <analysis>任務分��</analysis>
  <steps>
    <step><shell>命令1</shell></step>
    <step><shell>命令2</shell></step>
  </steps>
  <decision>continue</decision>
</plan>"""

    response = await call_ollama(prompt, PLANNER_SYSTEM)
    
    if DEBUG:
        print(f"[DEBUG] Planner improve response: {response[:300]}")
    
    new_plan = Plan()
    new_plan.decision = "continue"
    
    analysis_match = re.search(r'<analysis>(.*?)</analysis>', response, re.DOTALL)
    if analysis_match:
        new_plan.analysis = analysis_match.group(1).strip()
    
    step_matches = re.findall(r'<step>(.*?)</step>', response, re.DOTALL)
    new_plan.steps = []
    for s in step_matches:
        step = s.strip()
        if '<shell>' in step:
            new_plan.steps.append(step)
        else:
            new_plan.steps.append(f"<shell>{step}</shell>")
    
    if not new_plan.steps:
        shell_matches = re.findall(r'<shell>(.*?)</shell>', response, re.DOTALL)
        new_plan.steps = [f"<shell>{s.strip()}</shell>" for s in shell_matches]
    
    new_plan.feedback = feedback
    
    append_memory("planner", 0, f"迭代 {iteration}: {new_plan.analysis[:50]}", iteration)
    
    return "continue", new_plan

# ─── Generator ───

GENERATOR_SYSTEM = """你是 Generator，團隊的執行者。

職責：
1. 按照 Planner 的計劃執行操作
2. 調用工具完成任務
3. 報告執行結果

重要規則：
- Planner 已經用 <shell> 標籤包住命令，你只需要執行它們
- 用 <output> 標籤報告結果
- 完成後用 <done/> 結束"""

async def execute_plan(plan: Plan, context: str, generator_id: int = 0) -> dict:
    """Execute plan using Agent0"""
    agent = get_agent()
    outputs = []
    all_output = ""
    
    for i, step in enumerate(plan.steps):
        cmd_match = re.search(r'<shell>(.*?)</shell>', step, re.DOTALL)
        if not cmd_match:
            output = "No shell command found"
            outputs.append({"step_index": i, "command": "", "output": output, "success": False})
            all_output += f"\n=== 步驟 {i+1} ===\n{output}\n"
            continue
        
        cmd = cmd_match.group(1).strip()
        
        returncode, output = await agent.execute_shell(cmd)
        
        outputs.append({
            "step_index": i,
            "command": cmd,
            "output": output,
            "success": returncode == 0
        })
        all_output += f"\n=== 步驟 {i+1} ===\n$ {cmd}\n\n結果：{output}\n"
    
    all_outputs_text = "\n".join([
        f"步驟 {o['step_index']+1}: {o['output'][:200]}"
        for o in outputs
    ])
    append_memory("generator", generator_id, all_outputs_text, current_iteration)
    
    write_shared(f"output_{generator_id}.txt", all_output)
    
    return {
        "generator_id": generator_id,
        "success": all(o["success"] for o in outputs),
        "outputs": outputs,
        "full_output": all_output
    }

# ─── Evaluator ───

EVALUATOR_SYSTEM = """你是 Evaluator，團隊的評估者。

職責：
1. 評估 Generator 的輸出是否滿足要求
2. 提供具體的反饋
3. 決定是否需要重試

重要規則：
- 用 <evaluation> 標籤輸出評估結果
- <result> 為 PASS 或 FAIL
- <score> 為 0-10 的評分
- <feedback> 提供具體改進建議
- 如果 PASS，用 <end/> 結束"""

async def evaluate_output(user_input: str, generator_outputs: list, plan: Plan, evaluator_id: int = 0) -> dict:
    """Evaluate Generator output"""
    outputs_text = "\n".join([
        f"Generator {o['generator_id']}:\n{o['full_output'][:500]}"
        for o in generator_outputs
    ])
    
    prompt = f"""<task>{user_input}</task>

<plan>{plan.analysis}</plan>

<generator_outputs>
{outputs_text}
</generator_outputs>

請評估以上輸出是否滿足任務要求："""

    response = await call_ollama(prompt, EVALUATOR_SYSTEM)
    
    if DEBUG:
        print(f"[DEBUG] Evaluator response: {response[:300]}")
    
    result_match = re.search(r'<result>(.*?)</result>', response, re.DOTALL)
    passed = result_match.group(1).strip().upper() == "PASS" if result_match else False
    
    score_match = re.search(r'<score>(\d+)</score>', response, re.DOTALL)
    score = int(score_match.group(1)) if score_match else 5
    
    feedback_match = re.search(r'<feedback>(.*?)</feedback>', response, re.DOTALL)
    feedback = feedback_match.group(1).strip() if feedback_match else ""
    
    issue_match = re.findall(r'<issue>(.*?)</issue>', response, re.DOTALL)
    issues = [i.strip() for i in issue_match]
    
    append_memory("evaluator", evaluator_id, f"評估: {feedback[:100]}", current_iteration)
    write_shared(f"evaluation_{evaluator_id}.txt", response)
    
    return {
        "evaluator_id": evaluator_id,
        "passed": passed,
        "score": score,
        "feedback": feedback,
        "issues": issues,
        "raw_response": response
    }

# ─── Team Loop ───

def log_team(action: str, data: str = ""):
    """Log team action"""
    timestamp = datetime.now().strftime("%H:%M:%S")
    log_line = f"[{timestamp}] {action}: {data}"
    append_shared("teamlog.txt", log_line)
    team_log.append({"time": timestamp, "action": action, "data": data})
    if DEBUG:
        print(f"[DEBUG] {log_line}")

async def run_team_loop(user_input: str) -> tuple[str, list]:
    """Main team loop"""
    global current_iteration
    current_iteration = 0
    team_log.clear()
    
    log_team("START", user_input[:50])
    
    context = build_context()
    plan = None
    generators_outputs = []
    evaluators_results = []
    
    while current_iteration < MAX_ITERATIONS:
        log_team(f"ITERATION", str(current_iteration + 1))
        
        feedback = None
        if plan and plan.feedback:
            feedback = plan.feedback
        
        plan = await plan_task(user_input, context, feedback)
        
        if DEBUG:
            print(f"[DEBUG] Plan: {plan.analysis[:100]}")
            print(f"[DEBUG] Steps: {len(plan.steps)}")
        
        if plan.decision == "complete":
            log_team("COMPLETE", "task completed")
            break
        
        if plan.decision == "abandon":
            log_team("ABANDON", plan.analysis[:50])
            break
        
        generators_outputs = []
        for gen_id in range(NUM_GENERATORS):
            output = await execute_plan(plan, context, gen_id)
            generators_outputs.append(output)
        
        evaluators_results = []
        for eval_id in range(NUM_EVALUATORS):
            result = await evaluate_output(user_input, generators_outputs, plan, eval_id)
            evaluators_results.append(result)
        
        review_result = await review_evaluations(evaluators_results, plan)
        
        decision, new_plan = await decide_next_plan(review_result, plan, current_iteration)
        
        if decision == "complete":
            log_team("COMPLETE", "all evaluations passed")
            plan = new_plan
            break
        
        if decision == "abandon":
            log_team("ABANDON", "max iterations reached")
            plan = new_plan
            break
        
        plan = new_plan
        current_iteration += 1
    
    final_output = "\n".join([
        f"Generator {o['generator_id']}: {o['full_output'][:300]}"
        for o in generators_outputs
    ])
    
    log_team("END", "")
    
    return final_output, team_log

# ─── Main ───

def main():
    global NUM_GENERATORS, NUM_EVALUATORS, MAX_ITERATIONS, DEBUG, current_task_id
    
    parser = argparse.ArgumentParser(description="Agent0Team")
    parser.add_argument("--generators", type=int, default=1, help="Number of generators")
    parser.add_argument("--evaluators", type=int, default=1, help="Number of evaluators")
    parser.add_argument("--max-iterations", type=int, default=3, help="Max iterations")
    parser.add_argument("--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()
    
    NUM_GENERATORS = args.generators
    NUM_EVALUATORS = args.evaluators
    MAX_ITERATIONS = args.max_iterations
    DEBUG = args.debug
    
    os.makedirs(WORKSPACE, exist_ok=True)
    os.makedirs(MEMORY_DIR, exist_ok=True)
    os.makedirs(SHARED_DIR, exist_ok=True)
    
    current_task_id = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    print(f"Agent0Team - {MODEL}")
    print(f"Generators: {NUM_GENERATORS}, Evaluators: {NUM_EVALUATORS}, Max Iterations: {MAX_ITERATIONS}")
    print(f"工作區：{WORKSPACE}")
    print(f"Task ID: {current_task_id}")
    print("指令：/quit、/memory（顯示關鍵資訊）\n")
    
    while True:
        try:
            user_input = input("你：").strip()
        except (EOFError, KeyboardInterrupt):
            print("\n再見！")
            break
        
        if not user_input:
            continue
        if user_input.lower() in ["/quit", "/exit", "/q"]:
            print("再見！")
            break
        if user_input.lower() == "/memory":
            print(f"關鍵資訊：{key_info}")
            print(f"團隊記憶：{team_log[-5:]}")
            continue
        
        response, log = asyncio.run(run_team_loop(user_input))
        
        print(f"\n🤖 [任務完成]\n{response[:500]}\n")
        
        if DEBUG:
            print("═══ 團隊執行記錄 ═══")
            for entry in log[-10:]:
                print(f"  [{entry['time']}] {entry['action']}: {entry['data'][:50]}")
            print("═══")
        
        update_memory(user_input, response[:200])
        if response:
            asyncio.run(extract_key_info(user_input, response[:200]))

if __name__ == "__main__":
    main()