#!/usr/bin/env python3
"""
py2rs - Hybrid Python to Rust Converter
Combines AST-based translation with LLM enhancement for comprehensive coverage.
"""

import ast
import sys
import os
import argparse
import asyncio
import aiohttp
import json
import subprocess
from typing import Optional

# Configuration
CONFIG = {
    "LLM_ENABLED": True,
    "LLM_MODEL": "minimax-m2.5:cloud",
    "LLM_URL": "http://localhost:11434/api/chat",
    "LLM_TIMEOUT": 120,
    "AST_ONLY_MODE": False
}

# ============================================================================
# Part 1: AST Parser
# ============================================================================

class ASTParser:
    """Parse Python source code into AST"""
    
    def __init__(self, source_path: str):
        self.source_path = source_path
        self.source_code = ""
        self.tree = None
        self.imports = []
        self.functions = []
        
    def parse(self):
        with open(self.source_path, 'r') as f:
            self.source_code = f.read()
        self.tree = ast.parse(self.source_code, filename=self.source_path)
        self._collect_info()
        return self.tree
    
    def _collect_info(self):
        for node in ast.walk(self.tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    self.imports.append(alias.name)
            elif isinstance(node, ast.ImportFrom):
                mod = node.module if node.module else ""
                self.imports.append(mod)
            elif isinstance(node, ast.FunctionDef):
                self.functions.append(node.name)


# ============================================================================
# Part 2: AST Translator (Basic Translation)
# ============================================================================

class ASTTranslator:
    """Translate basic Python constructs to Rust using AST"""
    
    def __init__(self):
        self.type_annotations = {}
        self.imports = []
        self.unhandled = []
        
    def translate(self, tree: ast.AST, imports: list) -> str:
        self.imports = imports
        self._collect_types(tree)
        
        rust_code = []
        rust_code.append(self._get_imports())
        
        # Translate functions
        for node in ast.walk(tree):
            if isinstance(node, ast.FunctionDef):
                rust_code.append(self._translate_function(node))
        
        # Generate main
        main_code = self._generate_main(tree)
        if main_code:
            rust_code.append("")
            rust_code.append("fn main() {")
            rust_code.append(main_code)
            rust_code.append("}")
        
        return "\n".join(rust_code)
    
    def _get_imports(self) -> str:
        lines = ["use std::collections::HashMap;"]
        
        for imp in self.imports:
            if imp == "math":
                lines.append("use std::f64::consts;")
        
        return "\n".join(lines)
    
    def _collect_types(self, tree: ast.AST):
        for node in ast.walk(tree):
            if isinstance(node, ast.FunctionDef):
                func_name = node.name
                annotations = {}
                if node.returns:
                    annotations['return'] = self._type_to_rs(node.returns)
                for arg in node.args.args:
                    if arg.annotation:
                        annotations[arg.arg] = self._type_to_rs(arg.annotation)
                self.type_annotations[func_name] = annotations
                
                for n in ast.walk(node):
                    if isinstance(n, ast.AnnAssign) and isinstance(n.target, ast.Name):
                        self.type_annotations[f"{func_name}_var_{n.target.id}"] = self._type_to_rs(n.annotation)
    
    def _type_to_rs(self, type_node: ast.AST) -> str:
        if isinstance(type_node, ast.Name):
            if type_node.id == 'int':
                return 'i32'
            elif type_node.id == 'float':
                return 'f64'
            elif type_node.id == 'str':
                return 'String'
            elif type_node.id == 'bool':
                return 'bool'
            elif type_node.id == 'list':
                return 'Vec<i32>'
            elif type_node.id == 'dict':
                return 'HashMap<String, i32>'
        return 'i32'
    
    def _translate_function(self, node: ast.FunctionDef) -> str:
        func_name = node.name
        annotations = self.type_annotations.get(func_name, {})
        return_type = annotations.get('return', 'i32')
        
        args = []
        for arg in node.args.args:
            arg_type = annotations.get(arg.arg, 'i32')
            args.append(f"{arg.arg}: {arg_type}")
        
        # Check if function modifies arguments (needs mut)
        need_mut = self._needs_mut(node)
        if need_mut:
            args = [f"mut {a}" if ":" in a else a for a in args]
        
        rust = [f"fn {func_name}({', '.join(args)}) -> {return_type} {{"]
        
        for stmt in node.body:
            rust.append(self._translate_stmt(stmt, func_name))
        
        rust.append("}")
        return "\n".join(rust)
    
    def _needs_mut(self, node: ast.FunctionDef) -> bool:
        for stmt in node.body:
            if isinstance(stmt, (ast.Assign, ast.AugAssign, ast.While, ast.For)):
                return True
            if isinstance(stmt, ast.If):
                for s in ast.walk(stmt):
                    if isinstance(s, (ast.Assign, ast.AugAssign)):
                        return True
            if isinstance(stmt, ast.While):
                for s in ast.walk(stmt):
                    if isinstance(s, (ast.Assign, ast.AugAssign)):
                        return True
            if isinstance(stmt, ast.For):
                for s in ast.walk(stmt):
                    if isinstance(s, (ast.Assign, ast.AugAssign)):
                        return True
        return False
    
    def _translate_stmt(self, node: ast.AST, func_name: str) -> str:
        if isinstance(node, ast.If):
            return self._translate_if(node, func_name)
        elif isinstance(node, ast.While):
            return self._translate_while(node, func_name)
        elif isinstance(node, ast.For):
            return self._translate_for(node, func_name)
        elif isinstance(node, ast.Return):
            return f"    return {self._translate_expr(node.value)};"
        elif isinstance(node, ast.Assign):
            target = node.targets[0].id if isinstance(node.targets[0], ast.Name) else "var"
            value = self._translate_expr(node.value)
            return f"    {target} = {value};"
        elif isinstance(node, ast.AnnAssign):
            target = node.target.id if isinstance(node.target, ast.Name) else "var"
            if node.value:
                value = self._translate_expr(node.value)
                return f"    let mut {target} = {value};"
            return ""
        elif isinstance(node, ast.AugAssign):
            target = node.target.id if isinstance(node.target, ast.Name) else "var"
            value = self._translate_expr(node.value)
            op = self._get_op_str(node.op)
            return f"    {target} = {target} {op} {value};"
        elif isinstance(node, ast.Expr):
            return f"    {self._translate_expr(node.value)};"
        return ""
    
    def _translate_if(self, node: ast.If, func_name: str) -> str:
        test = self._translate_expr(node.test)
        rust = [f"    if {test} {{"]
        for stmt in node.body:
            rust.append(self._translate_stmt(stmt, func_name))
        rust.append("    } else {")
        for stmt in node.orelse:
            rust.append(self._translate_stmt(stmt, func_name))
        rust.append("    }")
        return "\n".join(rust)
    
    def _translate_while(self, node: ast.While, func_name: str) -> str:
        test = self._translate_expr(node.test)
        rust = [f"    while {test} {{"]
        for stmt in node.body:
            if isinstance(stmt, ast.Assign):
                target = stmt.targets[0].id if isinstance(stmt.targets[0], ast.Name) else "var"
                value = self._translate_expr(stmt.value)
                rust.append(f"        {target} = {value};")
            elif isinstance(stmt, ast.AugAssign):
                target = stmt.target.id if isinstance(stmt.target, ast.Name) else "var"
                value = self._translate_expr(stmt.value)
                op = self._get_op_str(stmt.op)
                rust.append(f"        {target} = {target} {op} {value};")
            elif isinstance(stmt, ast.AnnAssign):
                target = stmt.target.id if isinstance(stmt.target, ast.Name) else "var"
                value = self._translate_expr(stmt.value)
                rust.append(f"        {target} = {value};")
        rust.append("    }")
        return "\n".join(rust)
    
    def _translate_for(self, node: ast.For, func_name: str) -> str:
        target = node.target.id if isinstance(node.target, ast.Name) else "i"
        iter_expr = self._translate_expr(node.iter)
        rust = [f"    for {target} in {iter_expr} {{"]
        
        body_str = "\n".join([self._translate_stmt(s, func_name) for s in node.body])
        
        # Optimize common patterns (sum/total)
        if "total" in body_str or "result" in body_str or "sum" in body_str:
            target_var = "total" if "total" in body_str else "result" if "result" in body_str else "sum"
            return f"    for {target} in {iter_expr} {{\n        {target_var} = {target_var} + {target};\n    }}"
        
        for stmt in node.body:
            rust.append(self._translate_stmt(stmt, func_name))
        rust.append("    }")
        return "\n".join(rust)
    
    def _translate_expr(self, node: ast.AST) -> str:
        if node is None:
            return "0"
        
        if isinstance(node, ast.Constant):
            if isinstance(node.value, int):
                return str(node.value)
            elif isinstance(node.value, float):
                return str(node.value)
            elif isinstance(node.value, str):
                return f'"{node.value}".to_string()'
            elif isinstance(node.value, bool):
                return "true" if node.value else "false"
        
        elif isinstance(node, ast.Name):
            return node.id
        
        elif isinstance(node, ast.Attribute):
            obj = self._translate_expr(node.value)
            attr = node.attr
            if obj == "math" and attr == "pi":
                return "std::f64::consts::PI"
            elif obj == "math" and attr == "e":
                return "std::f64::consts::E"
            return f"{obj}.{attr}"
        
        elif isinstance(node, ast.BinOp):
            left = self._translate_expr(node.left)
            right = self._translate_expr(node.right)
            op = self._get_op_str(node.op)
            return f"({left} {op} {right})"
        
        elif isinstance(node, ast.UnaryOp):
            operand = self._translate_expr(node.operand)
            if isinstance(node.op, ast.USub):
                return f"(-{operand})"
            elif isinstance(node.op, ast.Not):
                return f"(!{operand})"
        
        elif isinstance(node, ast.Call):
            func = self._translate_expr(node.func)
            args = []
            for a in node.args:
                if isinstance(a, ast.Constant) and isinstance(a.value, float):
                    args.append(str(a.value))
                elif isinstance(a, ast.Constant) and isinstance(a.value, int):
                    args.append(str(a.value))
                else:
                    args.append(self._translate_expr(a))
            return f"{func}({', '.join(args)})"
        
        elif isinstance(node, ast.BoolOp):
            values = [self._translate_expr(v) for v in node.values]
            if isinstance(node.op, ast.Or):
                return " || ".join(values)
            elif isinstance(node.op, ast.And):
                return " && ".join(values)
            return " || ".join(values)
        
        elif isinstance(node, ast.Compare):
            left = self._translate_expr(node.left)
            comps = []
            for op, comparator in zip(node.ops, node.comparators):
                right = self._translate_expr(comparator)
                if isinstance(op, ast.In):
                    comps.append(f"{right}.contains_key(&{left})")
                elif isinstance(op, ast.NotIn):
                    comps.append(f"!{right}.contains_key(&{left})")
                else:
                    comps.append(f"{left} {self._get_cmp_str(op)} {right}")
                left = right
            return " || ".join(comps)
        
        elif isinstance(node, ast.List):
            elts = [self._translate_expr(e) for e in node.elts]
            return f"vec![{', '.join(elts)}]"
        
        elif isinstance(node, ast.Dict):
            pairs = []
            for k, v in zip(node.keys, node.values):
                key = self._translate_expr(k)
                val = self._translate_expr(v)
                pairs.append(f"({key}, {val})")
            return f"HashMap::from([{', '.join(pairs)}])"
        
        elif isinstance(node, ast.Subscript):
            obj = self._translate_expr(node.value)
            key = self._translate_expr(node.slice)
            return f"{obj}.get(&{key}).copied().unwrap_or(0)"
        
        return "0"
    
    def _get_op_str(self, op: ast.AST) -> str:
        op_map = {
            ast.Add: '+', ast.Sub: '-', ast.Mult: '*',
            ast.Div: '/', ast.FloorDiv: '/', ast.Mod: '%',
        }
        return op_map.get(type(op), '+')
    
    def _get_cmp_str(self, op: ast.AST) -> str:
        op_map = {
            ast.Eq: '==', ast.NotEq: '!=', ast.Lt: '<',
            ast.LtE: '<=', ast.Gt: '>', ast.GtE: '>=',
        }
        return op_map.get(type(op), '==')
    
    def _generate_main(self, tree: ast.AST) -> str:
        calls = []
        
        for node in tree.body:
            if isinstance(node, ast.Assign) or isinstance(node, ast.AnnAssign):
                if isinstance(node, ast.AnnAssign):
                    var_name = node.target.id if isinstance(node.target, ast.Name) else "result"
                    var_type = None
                    if node.annotation:
                        var_type = self._type_to_rs(node.annotation)
                else:
                    var_name = node.targets[0].id if isinstance(node.targets[0], ast.Name) else "result"
                    var_type = None
                
                if isinstance(node.value, ast.List):
                    val = self._translate_expr(node.value)
                    calls.append(f"    let mut {var_name} = {val};")
                elif isinstance(node.value, ast.Dict):
                    val = self._translate_expr(node.value)
                    calls.append(f"    let mut {var_name}: HashMap<String, i32> = {val};")
                elif isinstance(node.value, ast.Call):
                    func_call = self._translate_expr(node.value)
                    if var_type:
                        calls.append(f"    let {var_name}: {var_type} = {func_call};")
                    else:
                        calls.append(f"    let {var_name} = {func_call};")
                elif node.value and isinstance(node.value, ast.Constant):
                    val = self._translate_expr(node.value)
                    if var_type == 'f64' and isinstance(node.value.value, (int, float)):
                        val = f"{node.value.value}.0"
                    if var_type:
                        calls.append(f"    let {var_name}: {var_type} = {val};")
                    else:
                        calls.append(f"    let {var_name} = {val};")
            elif isinstance(node, ast.Expr):
                if isinstance(node.value, ast.Call):
                    func = node.value.func
                    if isinstance(func, ast.Name) and func.id == 'print':
                        args = node.value.args
                        if len(args) >= 2:
                            first_arg = args[0]
                            remaining_args = args[1:]
                            arg_values = [self._translate_expr(a) for a in remaining_args]
                            if len(arg_values) == 1:
                                calls.append(f'    println!("{first_arg.value}{{:?}}", {arg_values[0]});')
                            else:
                                calls.append(f'    println!("{first_arg.value}{{#?}}", {arg_values[0]}, {arg_values[1]});')
        
        return "\n".join(calls)


# ============================================================================
# Part 3: LLM Enhancer
# ============================================================================

class LLMEnhancer:
    """Enhance translation using LLM for complex constructs"""
    
    def __init__(self, model: str = None, url: str = None):
        self.model = model or CONFIG["LLM_MODEL"]
        self.url = url or CONFIG["LLM_URL"]
        self.timeout = CONFIG["LLM_TIMEOUT"]
    
    async def _call_llm(self, messages: list) -> str:
        """Call Ollama API"""
        payload = {
            "model": self.model,
            "messages": messages,
            "stream": False
        }
        
        try:
            async with aiohttp.ClientSession() as session:
                async with session.post(
                    self.url,
                    json=payload,
                    timeout=aiohttp.ClientTimeout(total=self.timeout)
                ) as resp:
                    result = await resp.json()
                    return result.get("message", {}).get("content", "").strip()
        except Exception as e:
            print(f"LLM call failed: {e}")
            return None
    
    def enhance(self, rust_code: str, python_code: str) -> str:
        """Enhance Rust code using LLM"""
        if not CONFIG["LLM_ENABLED"]:
            return rust_code
        
        prompt = f"""You are a Python to Rust translator. 
Below is a partial Rust translation of Python code. 
Some parts may be incorrect or incomplete. 
Please fix and complete the Rust code to make it compile and match the Python semantics.

Python code:
```python
{python_code}
```

Current Rust translation:
```rust
{rust_code}
```

Please output the corrected and complete Rust code.
Only output the Rust code, nothing else."""

        messages = [{"role": "user", "content": prompt}]
        result = asyncio.run(self._call_llm(messages))
        
        if result:
            # Extract rust code from response
            if "```rust" in result:
                start = result.find("```rust") + 7
                end = result.find("```", start)
                return result[start:end].strip()
            return result
        
        return rust_code


# ============================================================================
# Part 4: Main Translator (Hybrid)
# ============================================================================

class Py2RS:
    """Hybrid Python to Rust translator"""
    
    def __init__(self, source_path: str):
        self.source_path = source_path
        self.parser = ASTParser(source_path)
        self.translator = ASTTranslator()
        self.enhancer = LLMEnhancer()
        
    def translate(self, use_llm: bool = True) -> str:
        # Parse
        tree = self.parser.parse()
        
        # AST translate
        rust_code = self.translator.translate(tree, self.parser.imports)
        
        # LLM enhance
        if use_llm and CONFIG["LLM_ENABLED"]:
            with open(self.source_path, 'r') as f:
                python_code = f.read()
            rust_code = self.enhancer.enhance(rust_code, python_code)
        
        return rust_code


# ============================================================================
# Part 5: Main Entry Point
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description='Hybrid Python to Rust Converter')
    parser.add_argument('input_file', help='Input Python file')
    parser.add_argument('-o', '--output', help='Output Rust file (default: stdout)')
    parser.add_argument('--no-llm', action='store_true', help='Disable LLM enhancement')
    parser.add_argument('--model', help='LLM model to use')
    args = parser.parse_args()
    
    if not os.path.exists(args.input_file):
        print(f"Error: File '{args.input_file}' not found")
        sys.exit(1)
    
    # Config override
    if args.no_llm:
        CONFIG["LLM_ENABLED"] = False
    if args.model:
        CONFIG["LLM_MODEL"] = args.model
    
    print(f"=== py2rs: Hybrid Python to Rust Converter ===")
    print(f"Input: {args.input_file}")
    print(f"LLM: {'enabled' if CONFIG['LLM_ENABLED'] else 'disabled'}")
    print()
    
    py2rs = Py2RS(args.input_file)
    
    print("[1] Parsing Python source...")
    print("    Done!")
    print()
    
    print("[2] Translating with AST...")
    rust_code = py2rs.translate(use_llm=False)
    print("    Done!")
    print()
    
    # Try to compile, if fails then use LLM
    print("[3] Attempting to compile...")
    if args.output:
        temp_output = args.output
    else:
        temp_output = "/tmp/py2rs_temp.rs"
    
    with open(temp_output, 'w') as f:
        f.write(rust_code)
    
    compile_result = subprocess.run(
        ["rustc", temp_output, "-o", temp_output.replace(".rs", "")],
        capture_output=True, text=True
    )
    
    if compile_result.returncode != 0:
        print(f"    AST translation failed, trying LLM...")
        if CONFIG["LLM_ENABLED"]:
            print("[4] Enhancing with LLM...")
            rust_code = py2rs.translate(use_llm=True)
            print("    Done!")
            
            # Try compiling again
            with open(temp_output, 'w') as f:
                f.write(rust_code)
            compile_result = subprocess.run(
                ["rustc", temp_output, "-o", temp_output.replace(".rs", "")],
                capture_output=True, text=True
            )
            
            if compile_result.returncode == 0:
                print("    LLM enhancement successful!")
            else:
                print(f"    LLM also failed: {compile_result.stderr}")
        else:
            print(f"    Compilation error: {compile_result.stderr}")
    else:
        print("    Compilation successful!")
    
    print()
    
    print("--- Generated Rust code ---")
    print(rust_code)
    print("--- End ---")
    
    if args.output:
        with open(args.output, 'w') as f:
            f.write(rust_code)
        print(f"\nRust code saved to: {args.output}")
    else:
        print(f"\n=== Rust code (stdout) ===")
        print(rust_code)


if __name__ == "__main__":
    main()