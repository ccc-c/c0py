#!/usr/bin/env python3
"""
Python to C converter.
Reads Python source with type hints, generates C code.
Usage: python py2c.py <python_file.py> [-o output.c]
"""

import ast
import sys
import os
import argparse

class Py2C:
    def __init__(self, source_path: str):
        self.source_path = source_path
        self.functions = {}
        self.type_annotations = {}
        self.unsupported_features = []
        
    def parse(self):
        with open(self.source_path, 'r') as f:
            source = f.read()
        return ast.parse(source, filename=self.source_path)
    
    def check_unsupported(self, tree: ast.AST) -> list:
        unsupported = []
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    mod = alias.name
                    if mod not in []:  # empty list = no imports supported
                        unsupported.append(f"import {mod}")
            elif isinstance(node, ast.ImportFrom):
                mod = node.module if node.module else ""
                unsupported.append(f"from {mod} import ...")
            elif isinstance(node, ast.List):
                unsupported.append("list (陣列)")
            elif isinstance(node, ast.Dict):
                unsupported.append("dict (字典)")
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
            elif isinstance(node, ast.Subscript):
                if not isinstance(node.slice, ast.Index):
                    unsupported.append("subscript (切片)")
        return list(set(unsupported))
        
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
                    annotations['return'] = self._type_to_c(node.returns)
                for arg in node.args.args:
                    if arg.annotation:
                        annotations[arg.arg] = self._type_to_c(arg.annotation)
                self.type_annotations[func_name] = annotations
                
                for n in ast.walk(node):
                    if isinstance(n, ast.AnnAssign) and isinstance(n.target, ast.Name):
                        self.type_annotations[f"{func_name}_var_{n.target.id}"] = self._type_to_c(n.annotation)
    
    def _type_to_c(self, type_node: ast.AST) -> str:
        if isinstance(type_node, ast.Name):
            if type_node.id == 'int':
                return 'int'
            elif type_node.id == 'float':
                return 'double'
            elif type_node.id == 'str':
                return 'char*'
            elif type_node.id == 'bool':
                return 'int'
        return 'int'
    
    def compile_to_c(self, tree: ast.AST) -> str:
        c_code = []
        c_code.append("#include <stdio.h>")
        c_code.append("#include <stdlib.h>")
        c_code.append("")
        
        for node in ast.walk(tree):
            if isinstance(node, ast.FunctionDef):
                c_code.append(self._compile_function(node))
        
        main_call = self._generate_main_call(tree)
        if main_call:
            c_code.append("")
            c_code.append("int main() {")
            c_code.append(main_call)
            c_code.append("    return 0;")
            c_code.append("}")
        
        return "\n".join(c_code)
    
    def _compile_function(self, node: ast.FunctionDef) -> str:
        func_name = node.name
        annotations = self.type_annotations.get(func_name, {})
        return_type = annotations.get('return', 'int')
        
        args = []
        for arg in node.args.args:
            arg_type = annotations.get(arg.arg, 'int')
            args.append(f"{arg_type} {arg.arg}")
        
        c_code = []
        c_code.append(f"{return_type} {func_name}({', '.join(args)}) {{")
        
        local_vars = set()
        for stmt in node.body:
            if isinstance(stmt, ast.AnnAssign) and isinstance(stmt.target, ast.Name):
                local_vars.add(stmt.target.id)
        
        for var in local_vars:
            c_code.append(f"    int {var};")
        
        for stmt in node.body:
            c_code.append(self._compile_stmt(stmt, func_name))
                
        c_code.append("}")
        return "\n".join(c_code)
    
    def _compile_stmt(self, node: ast.AST, func_name: str) -> str:
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
                return f"    {target} = {value};"
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
        c_code = []
        c_code.append(f"    while ({test}) {{")
        for stmt in node.body:
            c_code.append(self._compile_stmt(stmt, func_name))
        c_code.append("    }")
        return "\n".join(c_code)
    
    def _compile_for(self, node: ast.For, func_name: str) -> str:
        target = node.target.id if isinstance(node.target, ast.Name) else "i"
        iter_expr = self._compile_expr(node.iter)
        c_code = []
        c_code.append(f"    for ({target} = 0; {target} < {iter_expr}; {target}++) {{")
        for stmt in node.body:
            c_code.append(self._compile_stmt(stmt, func_name))
        c_code.append("    }")
        return "\n".join(c_code)
    
    def _compile_if(self, node: ast.If, func_name: str) -> str:
        test = self._compile_expr(node.test)
        c_code = []
        c_code.append(f"    if ({test}) {{")
        
        for stmt in node.body:
            c_code.append(self._compile_stmt(stmt, func_name))
        
        c_code.append("    } else {")
        
        for stmt in node.orelse:
            c_code.append(self._compile_stmt(stmt, func_name))
        
        c_code.append("    }")
        return "\n".join(c_code)
    
    def _compile_expr(self, node: ast.AST) -> str:
        if node is None:
            return "0"
            
        if isinstance(node, ast.Constant):
            if isinstance(node.value, int):
                return str(node.value)
            elif isinstance(node.value, str):
                return f'"{node.value}"'
                
        elif isinstance(node, ast.Name):
            return node.id
            
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
            args = [self._compile_expr(a) for a in node.args]
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
            for op, comparator in zip(node.ops, node.comparators):
                right = self._compile_expr(comparator)
                comps.append(f"{left} {self._get_cmp_str(op)} {right}")
                left = right
            return " || ".join(comps)
            
        return "0"
    
    def _get_op_str(self, op: ast.AST) -> str:
        op_map = {
            ast.Add: '+', ast.Sub: '-', ast.Mult: '*',
            ast.Div: '/', ast.FloorDiv: '/', ast.Mod: '%',
            ast.Pow: '**', ast.LShift: '<<', ast.RShift: '>>',
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
    
    def _generate_main_call(self, tree: ast.AST) -> str:
        calls = []
        
        for node in tree.body:
            if isinstance(node, ast.Assign) or isinstance(node, ast.AnnAssign):
                if isinstance(node, ast.AnnAssign):
                    var_name = node.target.id if isinstance(node.target, ast.Name) else "result"
                else:
                    var_name = node.targets[0].id if isinstance(node.targets[0], ast.Name) else "result"
                if isinstance(node.value, ast.Call):
                    func_call = self._compile_expr(node.value)
                    calls.append(f"    int {var_name} = {func_call};")
                elif node.value and isinstance(node.value, ast.Constant):
                    calls.append(f"    int {var_name} = {self._compile_expr(node.value)};")
            elif isinstance(node, ast.Expr):
                if isinstance(node.value, ast.Call):
                    func = node.value.func
                    if isinstance(func, ast.Name) and func.id == 'print':
                        args = node.value.args
                        if len(args) >= 2:
                            first_arg = args[0]
                            if isinstance(first_arg, ast.Constant):
                                format_str = first_arg.value
                                remaining_args = args[1:]
                                arg_values = []
                                for arg in remaining_args:
                                    arg_values.append(self._compile_expr(arg))
                                if len(arg_values) == 1:
                                    calls.append(f'    printf("{format_str}%d\\n", {arg_values[0]});')
                                else:
                                    calls.append(f'    printf("{format_str}%d %d\\n", {arg_values[0]}, {arg_values[1]});')
                    else:
                        calls.append(f"    {self._compile_expr(node.value)};")
        
        return "\n".join(calls)

def main():
    parser = argparse.ArgumentParser(description='Convert Python to C')
    parser.add_argument('input_file', help='Input Python file')
    parser.add_argument('-o', '--output', help='Output C file (default: stdout)')
    args = parser.parse_args()
    
    if not os.path.exists(args.input_file):
        print(f"Error: File '{args.input_file}' not found")
        sys.exit(1)
    
    print(f"=== py2c: Python to C Converter ===")
    print(f"Input: {args.input_file}")
    print()
    
    py2c = Py2C(args.input_file)
    
    print("[1] Parsing Python source...")
    tree = py2c.parse()
    print("    Done!")
    
    print("[2] Checking for unsupported features...")
    unsupported = py2c.check_unsupported(tree)
    if unsupported:
        print("    ERROR: This program contains features that cannot be converted to C:")
        for feature in unsupported:
            print(f"      - {feature}")
        print()
        print("=== Conversion Failed ===")
        sys.exit(1)
    print("    All features supported!")
    print()
    
    print("[3] Collecting type annotations...")
    py2c.collect_types(tree)
    print(f"    Found types: {py2c.type_annotations}")
    print()
    
    print("[4] Generating C code...")
    c_code = py2c.compile_to_c(tree)
    print("--- Generated C code ---")
    print(c_code)
    print("--- End ---")
    
    if args.output:
        with open(args.output, 'w') as f:
            f.write(c_code)
        print(f"\nC code saved to: {args.output}")
    else:
        print(f"\n=== C code (stdout) ===")
        print(c_code)

if __name__ == "__main__":
    main()