#!/usr/bin/env python3
"""
The Ultimate WCS Auditor (Objdump + AST Bridges)
------------------------------------------------
Uses objdump for mathematically perfect direct-call resolution,
ingests AST-generated bridges.json for perfect indirect-call resolution,
and maps every function back to its original .c source file.
Displays the Top 3 heaviest call paths.
"""

import sys
import os
import subprocess
import re
import argparse
import shutil
import json
import glob
from typing import Dict, Set, List, Tuple

SYSCALL_WRAPPERS = {"SVC_Call", "SVC_cx_call"}

def check_dependencies(tool_name: str) -> None:
    if not shutil.which(tool_name):
        print(f"Error: '{tool_name}' not found in PATH.", file=sys.stderr)
        sys.exit(1)

def parse_su_files(search_dir: str) -> Dict[str, Tuple[int, str]]:
    """Parses .su files to extract both the stack weight AND the source file name."""
    print(f"Scanning for .su files in {search_dir}...")

    # Maps: function_name -> (stack_weight, source_file_name)
    stack_info: Dict[str, Tuple[int, str]] = {}
    su_files = glob.glob(os.path.join(search_dir, '**', '*.su'), recursive=True)

    for su_file in su_files:
        with open(su_file, 'r') as f:
            for line in f:
                parts = line.strip().split('\t')
                if len(parts) >= 2:
                    # GCC .su format is typically: path/to/file.c:line:col:func_name
                    location_parts = parts[0].split(':')
                    func_name = location_parts[-1]

                    # Extract just the file name (e.g., 'main.c' instead of '../src/main.c')
                    if len(location_parts) > 1:
                        file_name = os.path.basename(location_parts[0])
                    else:
                        file_name = "unknown.c"

                    try:
                        weight = int(parts[1])
                        stack_info[func_name] = (weight, file_name)
                    except ValueError:
                        pass

    print(f" -> Found stack info for {len(stack_info)} functions.\n")
    return stack_info

def parse_elf_graph(elf_path: str, objdump_tool: str) -> Dict[str, Set[str]]:
    print(f"Parsing direct branches from {elf_path} using objdump...")
    result = subprocess.run(
        [objdump_tool, "-d", elf_path], capture_output=True, text=True, check=True
    )

    graph: Dict[str, Set[str]] = {}
    current_func = None
    func_re = re.compile(r"^[0-9a-fA-F]+\s+<([^>]+)>:")
    call_re = re.compile(r"\s+b[lx]?\s+[0-9a-fA-F]+\s+<([^>+]+)(?:\+0x[0-9a-fA-F]+)?>")

    for line in result.stdout.splitlines():
        func_match = func_re.match(line)
        if func_match:
            current_func = func_match.group(1)
            if current_func not in graph:
                graph[current_func] = set()
            continue

        if current_func:
            call_match = call_re.search(line)
            if call_match:
                graph[current_func].add(call_match.group(1))

    print(f" -> Built base graph with {len(graph)} nodes and perfect direct calls.\n")
    return graph

def apply_ast_bridges(graph: Dict[str, Set[str]], bridges_file: str) -> None:
    if not os.path.exists(bridges_file):
        print(f" -> No '{bridges_file}' found. Skipping indirect call bridges.\n")
        return

    print(f"Injecting AST bridges from {bridges_file} to resolve indirect calls...")
    try:
        with open(bridges_file, 'r') as f:
            bridges = json.load(f)

        if "force_edges" in bridges:
            edges_added = 0
            for caller, targets in bridges["force_edges"].items():
                if caller not in graph:
                    graph[caller] = set()

                for target in targets:
                    if target not in graph[caller]:
                        graph[caller].add(target)
                        edges_added += 1
            print(f" -> Successfully mapped {edges_added} indirect callbacks!\n")
    except Exception as e:
        print(f" -> Warning: Failed to parse {bridges_file}: {e}\n")

def print_syscall_wrappers(graph: Dict[str, Set[str]]) -> Set[str]:
    print("=" * 70)
    print("SECTION 1: SYSCALL WRAPPERS IDENTIFIED")
    print("=" * 70)

    targets = set()
    idx = 1
    for caller in sorted(graph.keys()):
        used_wrappers = graph[caller].intersection(SYSCALL_WRAPPERS)
        if used_wrappers:
            targets.add(caller)
            for wrapper in used_wrappers:
                print(f" [{idx}]\t{caller}() -> calls {wrapper}")
                idx += 1

    if not targets:
        print("No functions calling syscall wrappers found.")
    return targets

def print_top_stack_paths(
    graph: Dict[str, Set[str]],
    targets: Set[str],
    stack_info: Dict[str, Tuple[int, str]]
) -> None:
    print("\n" + "=" * 70)
    print("SECTION 2: TOP 3 HEAVIEST STACK CONSUMPTION PATHS")
    print("=" * 70)

    memo: Dict[str, List[Tuple[int, List[str]]]] = {}
    visiting: Set[str] = set()

    def get_all_paths(node: str) -> List[Tuple[int, List[str]]]:
        if node in visiting:
            return []
        if node in memo:
            return memo[node]

        visiting.add(node)

        # Unpack the weight from our new tuple structure (defaults to 0 if not found)
        node_weight = stack_info.get(node, (0, ""))[0]

        all_child_paths = []

        if node in targets:
            # Base case: Hit a syscall, weight is 0 from here down
            all_child_paths.append((0, []))

        for neighbor in sorted(graph.get(node, [])):
            neighbor_paths = get_all_paths(neighbor)
            all_child_paths.extend(neighbor_paths)

        visiting.remove(node)

        # Build paths for this node
        final_paths = []
        for child_bytes, child_path in all_child_paths:
            total_bytes = node_weight + child_bytes
            final_path = [node] + child_path
            final_paths.append((total_bytes, final_path))

        # Sort and keep only the top 3 paths to prevent memory explosion
        final_paths.sort(key=lambda x: x[0], reverse=True)
        final_paths = final_paths[:3]

        memo[node] = final_paths
        return final_paths

    entry_points = ["main", "app_main", "Reset_Handler", "_start"]
    start_nodes = [n for n in entry_points if n in graph]

    if not start_nodes:
        start_nodes = sorted(graph.keys())

    print("Calculating paths via dynamic programming...")

    all_valid_paths = []
    for start_node in start_nodes:
        paths_from_node = get_all_paths(start_node)
        all_valid_paths.extend(paths_from_node)

    # Sort all collected paths globally and slice the top 3
    all_valid_paths.sort(key=lambda x: x[0], reverse=True)

    # Filter out exact duplicate paths that might arise from different entry points
    unique_paths = []
    seen_signatures = set()
    for weight, path in all_valid_paths:
        path_sig = tuple(path)
        if path_sig not in seen_signatures:
            seen_signatures.add(path_sig)
            unique_paths.append((weight, path))
            if len(unique_paths) == 3:
                break

    if unique_paths:
        for rank, (total_bytes, path) in enumerate(unique_paths, 1):
            if rank == 1:
                print(f"\n\033[91m[{rank}] WORST-CASE STACK USAGE (WCSU): {total_bytes} Bytes\033[0m")
            elif rank == 2:
                print(f"\n\033[93m[{rank}] 2nd Heaviest Path: {total_bytes} Bytes\033[0m")
            else:
                print(f"\n\033[93m[{rank}] 3rd Heaviest Path: {total_bytes} Bytes\033[0m")

            print("Path Breakdown:")

            running_total = 0
            for fn in path:
                w, file_name = stack_info.get(fn, (0, "unknown"))
                running_total += w

                fn_display = f"{fn} \033[36m[{file_name}]\033[0m"
                # Adjusted padding to 65
                print(f"  -> {fn_display.ljust(65)} \033[90m(+{str(w).rjust(3)}B) [Total: {str(running_total).rjust(4)}B]\033[0m")

            print(f"  -> \033[96m[SYSCALL WRAPPER]\033[0m")
    else:
        print("\nNo valid paths to syscall wrappers were found.")

    print("\n" + "=" * 70 + "\n")

def main():
    parser = argparse.ArgumentParser(description="The Ultimate WCS Auditor")
    parser.add_argument("elf_file", help="Path to the ARM ELF binary")
    parser.add_argument("--su-dir", default=".", help="Directory to scan for .su files")
    parser.add_argument("--bridges", default="bridges.json", help="Path to AST bridges file")
    parser.add_argument("--objdump", default="arm-none-eabi-objdump", help="Path to objdump tool")
    args = parser.parse_args()

    check_dependencies(args.objdump)

    # 1. Parse .su files (now returns both stack weight and file names)
    stack_info = parse_su_files(args.su_dir)

    # 2. Parse direct branches natively
    graph = parse_elf_graph(args.elf_file, args.objdump)

    # 3. Apply the AST hints for indirect branches
    apply_ast_bridges(graph, args.bridges)

    # 4. Find the 3 heaviest path
    targets = print_syscall_wrappers(graph)
    print_top_stack_paths(graph, targets, stack_info)

if __name__ == "__main__":
    main()
