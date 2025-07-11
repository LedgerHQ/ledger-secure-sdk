import re
import sys

NUM_WRITTEN_MOCKS = 0
NUM_SKIPPED_MOCKS = 0

def mark_params_unused(signature: str) -> str:
    global NUM_WRITTEN_MOCKS
    """Add __attribute__((unused)) to each function parameter name in the signature."""
    if '(' not in signature or ')' not in signature:
        return signature  # Not a valid function

    prefix, param_str = signature.split('(', 1)
    param_str, suffix = param_str.rsplit(')', 1)
    params = param_str.split(',')
    NUM_WRITTEN_MOCKS += 1

    def modify_param(param):
        param = param.strip()
        if not param or param == 'void':
            return param
        if '__attribute__' in param:
            return param  # Already marked
        # Attempt to find the variable name (last token)
        parts = param.rsplit(' ', 1)
        if len(parts) == 2:
            return f"{parts[0]} {parts[1]} __attribute__((unused))"
        else:
            return f"{param} __attribute__((unused))"

    modified_params = [modify_param(p) for p in params]
    return f"{prefix}({', '.join(modified_params)}){suffix}"

def matching(line):
    """Pattern matching for functions or defines"""
    case = -1
    line = line.strip()

    if line.startswith("#") or line == "":
        case = "#"

    elif re.match(r'^\s*[\w\s\*\_]+[*\s]+\w+\s*\(.*', line):
        case = "function"

    return case

def parse_defines(defines):
    """Parse the list of defined macros."""
    return set(defines.split())

def skip_function(line, lines, i):
    """ Skip lines until the closing brace is found """
    brace_counter = line.count('{') - line.count('}')
    i += 1

    while brace_counter > 0 and i < len(lines):
        line = lines[i]
        brace_counter += line.count('{') - line.count('}')
        i += 1

    return i - 1  # return to the last valid line

def gen_mocks(c_code):
    global NUM_SKIPPED_MOCKS
    skipped_lines = []
    write_lines = []
    write_lines.append('#ifndef __weak')
    write_lines.append('#define __weak __attribute__((weak))')
    write_lines.append('#endif')
    write_lines.append('#include "ox_aes.h"')
    lines = c_code.splitlines()
    i = 0

    while i < len(lines):
        line = lines[i]
        case = matching(line)

        match case:
            case "#":
                write_lines.append(line)

            case "function":

                if ';' in line:
                    write_lines.append(line)
                else:
                    line_aux = line.strip()
                    while ')' not in line and i < len(lines) - 1:
                        i += 1
                        line = lines[i]
                        line_aux += ' ' + line.strip()

                    if ')' in line_aux:
                        if line_aux.endswith(';'):  # prototype
                            unused_signature = mark_params_unused(line_aux)
                            write_lines.append('__weak ' + unused_signature)
                        elif line_aux.startswith('void'):
                            if 'noreturn' in line_aux:
                                write_lines.append('__weak '+ mark_params_unused(line_aux) + ' { __builtin_unreachable(); }')
                            else:
                                write_lines.append('__weak '+ mark_params_unused(line_aux) + ' { return; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('unsigned int') or 'SVC_cx_call' in line_aux:
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('cx_err_t'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0x00000000; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bolos_err_t'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0xaa; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bolos_bool_t'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0xaa; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bool'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return true; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bolos_bool_t'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0xaa; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bolos_task_status_t'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0x0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('uint8_t'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('try_context_t *'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return NULL; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('uint32_t'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('_Bool'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return true; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('unsigned char'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return \'M\'; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('unsigned short'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('int'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('const LANGUAGE_PACK *'):
                            write_lines.append('__weak ' + mark_params_unused(line_aux) + ' { return NULL; }')
                            i = skip_function(line, lines, i)
                        else:
                            print(f"Skipped line [{i+1}]: {line_aux}")
                            i = skip_function(line, lines, i)

            case _:
                # Other not identified lines
                if '{' in line:
                    i = skip_function(line, lines, i)
                else:
                    NUM_SKIPPED_MOCKS +=1
                    skipped_lines.append(f"Ignoring line[{i+1}] = {line}")

        i += 1

    return write_lines, skipped_lines

def main():


    WARNING = '\033[93m'
    ENDC = '\033[0m'
    if(len(sys.argv) > 2):
        functions_path = sys.argv[1]
        output_file = sys.argv[2]

        with open(functions_path, "r") as file:
            c_code = file.read()
            write_lines, skipped_lines = gen_mocks(c_code)

        if write_lines:
            with open(output_file, "w") as file:
                for line in write_lines:
                    file.write(line + "\n")
        skipped_file = output_file.split('.')[0]+"_skipped.txt"
        if skipped_lines:
            with open(skipped_file, "w") as file:
                for line in skipped_lines:
                    file.write(line + "\n")

    else:
        print("Usage: python3 gen_mock.py [c_functions_file.c]")
    print(f"{WARNING}{NUM_WRITTEN_MOCKS} mocks generated at {output_file}{ENDC}")
    print(f"{WARNING}{NUM_SKIPPED_MOCKS} lines skipped, written at {skipped_file}{ENDC}")

if __name__ == "__main__":
    main()
