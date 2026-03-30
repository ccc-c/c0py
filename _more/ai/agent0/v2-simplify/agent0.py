#!/usr/bin/env python3
# 修改自：
#   https://gist.github.com/dabit3/86ee04a1c02c839409a02b20fe99a492
#   mini-openclaw.py - A minimal OpenClaw clone
# ccc 重新命名為：
#   agent0.py - AI Agent using Ollama
# Run: python agent0.py

import subprocess
import json
import os
import asyncio
import aiohttp
import re

# ─── Configuration ───

WORKSPACE = os.path.expanduser("~/.agent0")
MODEL = "minimax-m2.5:cloud"

# ─── Tools ───

TOOLS = [
    {
        "name": "run_command",
        "description": "Run a shell command (use 'cat file' to read, 'echo content > file' or 'printf ... > file' to write)",
        "input_schema": {
            "type": "object",
            "properties": {
                "command": {"type": "string", "description": "The command to run"}
            },
            "required": ["command"]
        }
    },
]

# ─── Ollama API ───

async def call_ollama(prompt: str, system: str = "") -> str:
    """Call Ollama API"""
    full_prompt = f"{system}\n\n{prompt}" if system else prompt
    
    payload = {
        "model": MODEL,
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

# ─── Tool Execution ───

def execute_tool(name, tool_input):
    if name == "run_command":
        try:
            result = subprocess.run(
                tool_input["command"], shell=True, 
                capture_output=True, text=True, timeout=30
            )
            output = result.stdout + result.stderr
            return output if output else "(no output)"
        except Exception as e:
            return f"Error: {e}"
    
    return f"Unknown tool: {name}"

# ─── Agent ───

SYSTEM_PROMPT = """You are Jarvis, a helpful AI assistant.
You have tools - use them to help the user.

Available tools:
- run_command: Run a shell command. Use 'cat file' to read files, 'echo content > file' or 'printf' to write files.

When you need to use a tool, output in this format:
<tool>
{"name": "tool_name", "input": {"key": "value"}}
</tool>

Otherwise, just respond directly."""

def main():
    os.makedirs(WORKSPACE, exist_ok=True)
    
    print(f"Agent0 - {MODEL}")
    print(f"Workspace: {WORKSPACE}")
    print("Commands: /quit\n")
    
    while True:
        try:
            user_input = input("You: ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nGoodbye!")
            break
        
        if not user_input:
            continue
        if user_input.lower() in ["/quit", "/exit", "/q"]:
            print("Goodbye!")
            break
        
        # 调用 Ollama
        response = asyncio.run(call_ollama(user_input, SYSTEM_PROMPT))
        
        # 检查是否需要使用工具
        tool_match = re.search(r'<tool>(.+?)</tool>', response, re.DOTALL)
        if tool_match:
            try:
                tool_data = json.loads(tool_match.group(1))
                tool_name = tool_data.get("name")
                tool_input = tool_data.get("input", {})
                
                print(f"  🔧 {tool_name}: {tool_input}")
                
                result = execute_tool(tool_name, tool_input)
                print(f"     → {result[:150]}")
            except Exception as e:
                print(f"Tool error: {e}")
        else:
            print(f"\n🤖 {response}\n")

if __name__ == "__main__":
    main()
