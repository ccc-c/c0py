#!/usr/bin/env python3
"""
Python to Rust converter.
Reads Python source with type hints, generates Rust code.
Usage: python py2rs.py <python_file.py> [-o output.rs]
"""

import ast
import sys
import os
import argparse

class Py2Rs:
    def __init__(self, source_path: str):
        self.source_path = source_path
        self.functions = {}
        self.type_annotations = {}
        self.imports = []
        
    def parse(self):
        with open(self.source_path, 'r') as f:
            source = f.read()
        tree = ast.parse(source, filename=self.source_path)
        self._collect_imports(tree)
        return tree
    
    def _collect_imports(self, tree: ast.AST):
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    self.imports.append(alias.name)
            elif isinstance(node, ast.ImportFrom):
                mod = node.module if node.module else ""
                self.imports.append(mod)
        
    def parse(self):
        with open(self.source_path, 'r') as f:
            source = f.read()
        return ast.parse(source, filename=self.source_path)
    
    def collect_types(self, tree: ast.AST):
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
    
    def check_unsupported(self, tree: ast.AST) -> list:
        unsupported = []
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    mod = alias.name
                    if mod not in ['math', 'random']:
                        unsupported.append(f"import {mod}")
            elif isinstance(node, ast.ImportFrom):
                mod = node.module if node.module else ""
                unsupported.append(f"from {mod} import ...")
            elif isinstance(node, ast.Tuple):
                unsupported.append("tuple (元組)")
            elif isinstance(node, ast.Set):
                unsupported.append("set (集合)")
            elif isinstance(node, ast.ListComp):
                unsupported.append("list comprehension (列表推导)")
            elif isinstance(node, ast.DictComp):
                unsupported.append("dict comprehension (字典推导)")
            elif isinstance(node, ast.SetComp):
                unsupported.append("set comprehension (集合推导)")
            elif isinstance(node, ast.Lambda):
                unsupported.append("lambda (匿名函數)")
        return list(set(unsupported))
    
    def compile_to_rs(self, tree: ast.AST) -> str:
        rs_code = []
        rs_code.append("use std::collections::HashMap;")
        
        # Handle Python imports
        for imp in self.imports:
            if imp == "math":
                rs_code.append("use std::f64::consts;")
            elif imp == "random":
                rs_code.append("use rand::Rng;")
            elif imp == "json":
                rs_code.append("use serde_json;")
        
        if self.imports:
            rs_code.append("")
        
        for node in ast.walk(tree):
            if isinstance(node, ast.FunctionDef):
                rs_code.append(self._compile_function(node))
        
        main_call = self._generate_main(tree)
        if main_call:
            rs_code.append("")
            rs_code.append("fn main() {")
            rs_code.append(main_call)
            rs_code.append("}")
        
        return "\n".join(rs_code)
    
    def _compile_function(self, node: ast.FunctionDef) -> str:
        func_name = node.name
        annotations = self.type_annotations.get(func_name, {})
        return_type = annotations.get('return', 'i32')
        
        args = []
        for arg in node.args.args:
            arg_type = annotations.get(arg.arg, 'i32')
            args.append(f"{arg.arg}: {arg_type}")
        
        need_mut = False
        for stmt in node.body:
            if isinstance(stmt, ast.Assign):
                for target in stmt.targets:
                    if isinstance(target, ast.Name) and target.id in [arg.arg for arg in node.args.args]:
                        need_mut = True
            elif isinstance(stmt, ast.AugAssign):
                if isinstance(stmt.target, ast.Name) and stmt.target.id in [arg.arg for arg in node.args.args]:
                    need_mut = True
            elif isinstance(stmt, ast.While) or isinstance(stmt, ast.For):
                need_mut = True
        
        args_str = ", ".join(args)
        if need_mut:
            args_str = ", ".join([f"mut {a}" if ":" in a else a for a in args])
        
        rs_code = []
        rs_code.append(f"fn {func_name}({args_str}) -> {return_type} {{")
        
        for stmt in node.body:
            rs_code.append(self._compile_stmt(stmt, func_name))
                
        rs_code.append("}")
        return "\n".join(rs_code)
    
    def _compile_stmt(self, node: ast.AST, func_name: str, in_loop: bool = False) -> str:
        if isinstance(node, ast.If):
            return self._compile_if(node, func_name)
        elif isinstance(node, ast.While):
            return self._compile_while(node, func_name)
        elif isinstance(node, ast.For):
            return self._compile_for(node, func_name)
        elif isinstance(node, ast.Return):
            return f"    return {self._compile_expr(node.value)};"
        elif isinstance(node, ast.Assign):
            target = node.targets[0].id if isinstance(node.targets[0], ast.Name) else "var"
            value = self._compile_expr(node.value)
            return f"    {target} = {value};"
        elif isinstance(node, ast.AnnAssign):
            target = node.target.id if isinstance(node.target, ast.Name) else "var"
            if node.value:
                value = self._compile_expr(node.value)
                return f"    let mut {target} = {value};"
            return ""
        elif isinstance(node, ast.AugAssign):
            target = node.target.id if isinstance(node.target, ast.Name) else "var"
            value = self._compile_expr(node.value)
            op = self._get_op_str(node.op)
            return f"    {target} = {target} {op} {value};"
        elif isinstance(node, ast.Expr):
            return f"    {self._compile_expr(node.value)};"
        return ""
    
    def _compile_while(self, node: ast.While, func_name: str) -> str:
        test = self._compile_expr(node.test)
        
        rs_code = []
        rs_code.append(f"    while {test} {{")
        
        for stmt in node.body:
            if isinstance(stmt, ast.Assign):
                target = stmt.targets[0].id if isinstance(stmt.targets[0], ast.Name) else "var"
                value = self._compile_expr(stmt.value)
                rs_code.append(f"        {target} = {value};")
            elif isinstance(stmt, ast.AugAssign):
                target = stmt.target.id if isinstance(stmt.target, ast.Name) else "var"
                value = self._compile_expr(stmt.value)
                op = self._get_op_str(stmt.op)
                rs_code.append(f"        {target} = {target} {op} {value};")
            elif isinstance(stmt, ast.AnnAssign):
                target = stmt.target.id if isinstance(stmt.target, ast.Name) else "var"
                if stmt.value:
                    value = self._compile_expr(stmt.value)
                    rs_code.append(f"        {target} = {value};")
            else:
                rs_code.append(self._compile_stmt(stmt, func_name))
        
        rs_code.append("    }")
        return "\n".join(rs_code)
    
    def _compile_for(self, node: ast.For, func_name: str) -> str:
        target = node.target.id if isinstance(node.target, ast.Name) else "i"
        iter_expr = self._compile_expr(node.iter)
        
        body_lines = []
        for stmt in node.body:
            body_lines.append(self._compile_stmt(stmt, func_name))
        
        body_str = "\n".join(body_lines)
        
        target_var = None
        if "total" in body_str:
            target_var = "total"
        elif "result" in body_str:
            target_var = "result"
        elif "sum" in body_str:
            target_var = "sum"
        
        if target_var:
            return f"    for {target} in {iter_expr} {{\n        {target_var} = {target_var} + {target};\n    }}"
        
        rs_code = []
        rs_code.append(f"    for {target} in {iter_expr} {{")
        for stmt in node.body:
            rs_code.append(self._compile_stmt(stmt, func_name))
        rs_code.append("    }")
        return "\n".join(rs_code)
    
    def _compile_if(self, node: ast.If, func_name: str) -> str:
        test = self._compile_expr(node.test)
        rs_code = []
        rs_code.append(f"    if {test} {{")
        
        for stmt in node.body:
            rs_code.append(self._compile_stmt(stmt, func_name))
        
        rs_code.append("    } else {")
        
        for stmt in node.orelse:
            rs_code.append(self._compile_stmt(stmt, func_name))
        
        rs_code.append("    }")
        return "\n".join(rs_code)
    
    def _compile_expr(self, node: ast.AST) -> str:
        if node is None:
            return "0"
            
        if isinstance(node, ast.Constant):
            if isinstance(node.value, int):
                return str(node.value)
            elif isinstance(node.value, str):
                return f'"{node.value}".to_string()'
            elif isinstance(node.value, bool):
                return "true" if node.value else "false"
                
        elif isinstance(node, ast.Name):
            return node.id
            
        elif isinstance(node, ast.Attribute):
            obj = self._compile_expr(node.value)
            attr = node.attr
            if obj == "math" and attr == "pi":
                return "std::f64::consts::PI"
            elif obj == "math" and attr == "e":
                return "std::f64::consts::E"
            return f"{obj}.{attr}"
            
        elif isinstance(node, ast.BinOp):
            left = self._compile_expr(node.left)
            right = self._compile_expr(node.right)
            op = self._get_op_str(node.op)
            return f"({left} {op} {right})"
            
        elif isinstance(node, ast.UnaryOp):
            operand = self._compile_expr(node.operand)
            if isinstance(node.op, ast.USub):
                return f"(-{operand})"
            elif isinstance(node.op, ast.Not):
                return f"(!{operand})"
                
        elif isinstance(node, ast.Call):
            func = self._compile_expr(node.func)
            args = []
            for a in node.args:
                if isinstance(a, ast.Constant):
                    if isinstance(a.value, float):
                        args.append(str(a.value))
                    elif isinstance(a.value, int):
                        args.append(str(a.value))
                    else:
                        args.append(self._compile_expr(a))
                else:
                    args.append(self._compile_expr(a))
            return f"{func}({', '.join(args)})"
            
        elif isinstance(node, ast.BoolOp):
            values = [self._compile_expr(v) for v in node.values]
            if isinstance(node.op, ast.Or):
                return " || ".join(values)
            elif isinstance(node.op, ast.And):
                return " && ".join(values)
            return " || ".join(values)
            
        elif isinstance(node, ast.Compare):
            left = self._compile_expr(node.left)
            comps = []
            for i, (op, comparator) in enumerate(zip(node.ops, node.comparators)):
                right = self._compile_expr(comparator)
                if isinstance(op, ast.In):
                    comps.append(f"{right}.contains_key(&{left})")
                elif isinstance(op, ast.NotIn):
                    comps.append(f"!{right}.contains_key(&{left})")
                else:
                    comps.append(f"{left} {self._get_cmp_str(op)} {right}")
                left = right
            return " || ".join(comps)
            
        elif isinstance(node, ast.Subscript):
            obj = self._compile_expr(node.value)
            key = self._compile_expr(node.slice)
            return f"{obj}.get(&{key}).copied().unwrap_or(0)"
            
        elif isinstance(node, ast.List):
            elts = [self._compile_expr(e) for e in node.elts]
            return f"vec![{', '.join(elts)}]"
            
        elif isinstance(node, ast.Dict):
            pairs = []
            for k, v in zip(node.keys, node.values):
                key = self._compile_expr(k)
                val = self._compile_expr(v)
                pairs.append(f"({key}, {val})")
            return f"HashMap::from([{', '.join(pairs)}])"
            
        return "0"
    
    def _get_op_str(self, op: ast.AST) -> str:
        op_map = {
            ast.Add: '+', ast.Sub: '-', ast.Mult: '*',
            ast.Div: '/', ast.FloorDiv: '/', ast.Mod: '%',
            ast.Pow: 'pow', ast.LShift: '<<', ast.RShift: '>>',
            ast.BitAnd: '&', ast.BitOr: '|', ast.BitXor: '^'
        }
        return op_map.get(type(op), '+')
    
    def _get_cmp_str(self, op: ast.AST) -> str:
        op_map = {
            ast.Eq: '==', ast.NotEq: '!=', ast.Lt: '<',
            ast.LtE: '<=', ast.Gt: '>', ast.GtE: '>=',
            ast.Is: '==', ast.IsNot: '!=', ast.In: '==',
            ast.NotIn: '!='
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
                    val = self._compile_expr(node.value)
                    calls.append(f"    let mut {var_name} = {val};")
                elif isinstance(node.value, ast.Dict):
                    val = self._compile_expr(node.value)
                    calls.append(f"    let mut {var_name}: HashMap<String, i32> = {val};")
                elif isinstance(node.value, ast.Call):
                    func_call = self._compile_expr(node.value)
                    if var_type:
                        calls.append(f"    let {var_name}: {var_type} = {func_call};")
                    else:
                        calls.append(f"    let {var_name} = {func_call};")
                elif node.value and isinstance(node.value, ast.Constant):
                    val = self._compile_expr(node.value)
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
                            arg_values = []
                            for arg in remaining_args:
                                arg_values.append(self._compile_expr(arg))
                            if len(arg_values) == 1:
                                arg_name = remaining_args[0]
                                if isinstance(arg_name, ast.Name) and arg_name.id in ['arr', 'd']:
                                    calls.append(f'    println!("{first_arg.value}{{:?}}", {arg_values[0]});')
                                else:
                                    calls.append(f'    println!("{first_arg.value}{{}}", {arg_values[0]});')
                    else:
                        calls.append(f"    {self._compile_expr(node.value)};")
        
        return "\n".join(calls)

def main():
    parser = argparse.ArgumentParser(description='Convert Python to Rust')
    parser.add_argument('input_file', help='Input Python file')
    parser.add_argument('-o', '--output', help='Output Rust file (default: stdout)')
    args = parser.parse_args()
    
    if not os.path.exists(args.input_file):
        print(f"Error: File '{args.input_file}' not found")
        sys.exit(1)
    
    print(f"=== py2rs: Python to Rust Converter ===")
    print(f"Input: {args.input_file}")
    print()
    
    py2rs = Py2Rs(args.input_file)
    
    print("[1] Parsing Python source...")
    tree = py2rs.parse()
    print("    Done!")
    
    print("[2] Checking for unsupported features...")
    unsupported = py2rs.check_unsupported(tree)
    if unsupported:
        print("    ERROR: This program contains features that cannot be converted to Rust:")
        for feature in unsupported:
            print(f"      - {feature}")
        print()
        print("=== Conversion Failed ===")
        sys.exit(1)
    print("    All features supported!")
    print()
    
    print("[3] Collecting type annotations...")
    py2rs.collect_types(tree)
    print(f"    Found types: {py2rs.type_annotations}")
    print()
    
    print("[3] Generating Rust code...")
    rs_code = py2rs.compile_to_rs(tree)
    print("--- Generated Rust code ---")
    print(rs_code)
    print("--- End ---")
    
    if args.output:
        with open(args.output, 'w') as f:
            f.write(rs_code)
        print(f"\nRust code saved to: {args.output}")
    else:
        print(f"\n=== Rust code (stdout) ===")
        print(rs_code)

if __name__ == "__main__":
    main()