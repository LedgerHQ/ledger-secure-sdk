#!/usr/bin/env python3
"""
AST-to-Bridge Generator (Hybrid Analysis)
-----------------------------------------
Uses libclang and compile_commands.json to find all indirect callers
and address-taken functions. Matches them strictly by C function signature
and outputs a bridges.json file
"""

import os
import sys
import json
import argparse
import clang.cindex
from clang.cindex import CursorKind, TypeKind
from typing import Dict, Set

def get_signature(t: clang.cindex.Type) -> str:
    """Extracts a canonical C signature safely."""
    try:
        t = t.get_canonical()
        if t.kind == TypeKind.POINTER:
            t = t.get_pointee().get_canonical()

        # Strictly prototyped functions (e.g., void foo(int, int))
        if t.kind == TypeKind.FUNCTIONPROTO:
            ret_type = t.get_result().spelling
            args = [arg.spelling for arg in t.argument_types()]
            return f"{ret_type} ({', '.join(args)})"

        # Unprototyped functions (e.g., void foo())
        elif t.kind == TypeKind.FUNCTIONNOPROTO:
            ret_type = t.get_result().spelling
            return f"{ret_type} ()"

    except Exception:
        pass
    return None

def generate_ast_bridges(compile_cmds_path: str, output_file: str):
    print(f"Reading ASTs via {compile_cmds_path} to auto-generate bridges...")

    try:
        index = clang.cindex.Index.create()
    except Exception as e:
        print(f"Error initializing libclang: {e}", file=sys.stderr)
        sys.exit(1)

    with open(compile_cmds_path, 'r') as f:
        compdb = json.load(f)

    # Maps: Signature -> Set of Function Names
    indirect_callers: Dict[str, Set[str]] = {}
    address_taken_funcs: Dict[str, Set[str]] = {}

    total_files = len(compdb)
    for idx, entry in enumerate(compdb):
        sys.stdout.write(f"\r\033[K [AST] Processing file {idx + 1}/{total_files}...")
        sys.stdout.flush()

        file_path = os.path.abspath(os.path.join(entry.get('directory', ''), entry['file']))
        if not os.path.exists(file_path):
            continue

        raw_args = entry.get('arguments', entry.get('command', '').split())[1:]

        safe_args = []
        i = 0
        while i < len(raw_args):
            arg = raw_args[i]
            if arg.startswith('-D') or arg.startswith('-I') or arg.startswith('-isystem'):
                safe_args.append(arg)
                if arg in ('-D', '-I', '-isystem') and (i + 1) < len(raw_args):
                    i += 1
                    safe_args.append(raw_args[i])
            elif arg.startswith('-std='):
                safe_args.append(arg)
            i += 1

        safe_args.extend(['-x', 'c', '-Wno-everything'])

        try:
            tu = index.parse(file_path, args=safe_args)
            if not tu:
                continue
        except Exception:
            continue

        # Iterative AST traversal to avoid hitting Python's recursion limit
        # Stack items: (node, current_func, parent_kind)
        work_stack = [(tu.cursor, None, None)]
        while work_stack:
            node, current_func, parent_kind = work_stack.pop()

            if node.kind == CursorKind.FUNCTION_DECL and node.is_definition():
                current_func = node.spelling

            # 1. Address-Taken Functions (The Targets)
            if node.kind == CursorKind.DECL_REF_EXPR and node.referenced and node.referenced.kind == CursorKind.FUNCTION_DECL:
                if parent_kind != CursorKind.CALL_EXPR:
                    sig = get_signature(node.referenced.type)
                    if sig:
                        if sig not in address_taken_funcs:
                            address_taken_funcs[sig] = set()
                        address_taken_funcs[sig].add(node.referenced.spelling)

            # 2. Indirect Calls (The Dispatchers)
            if node.kind == CursorKind.CALL_EXPR:
                if not (node.referenced and node.referenced.kind == CursorKind.FUNCTION_DECL):
                    if current_func:
                        children = list(node.get_children())
                        if children:
                            callee = children[0]
                            sig = get_signature(callee.type)
                            if sig:
                                if sig not in indirect_callers:
                                    indirect_callers[sig] = set()
                                indirect_callers[sig].add(current_func)

            for child in node.get_children():
                work_stack.append((child, current_func, node.kind))

    sys.stdout.write("\n")

    # Build the JSON bridges mapping based on matching signatures
    bridge_data = {"force_edges": {}}
    edges_created = 0

    for sig, targets in address_taken_funcs.items():
        if sig in indirect_callers:
            for caller in indirect_callers[sig]:
                if caller not in bridge_data["force_edges"]:
                    bridge_data["force_edges"][caller] = []

                # Merge target lists and remove duplicates
                current_targets = set(bridge_data["force_edges"][caller])
                current_targets.update(targets)
                bridge_data["force_edges"][caller] = sorted(list(current_targets))

                edges_created += len(targets)

    print(f" -> Found {len(indirect_callers)} unique indirect call signatures.")
    print(f" -> Found {len(address_taken_funcs)} unique callback signatures.")
    print(f" -> Automatically synthesized {edges_created} highly-precise bridges.")

    with open(output_file, "w") as f:
        json.dump(bridge_data, f, indent=4)

    print(f"Successfully generated '{output_file}'!")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="AST to Bridge JSON Generator")
    parser.add_argument("compile_commands", help="Path to compile_commands.json")
    parser.add_argument("--out", default="bridges.json", help="Output JSON file path.")
    args = parser.parse_args()
    generate_ast_bridges(args.compile_commands, args.out)
