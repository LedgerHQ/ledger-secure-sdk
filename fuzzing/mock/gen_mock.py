import re
import sys
import subprocess

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
                            write_lines.append('__weak ' + line_aux)
                        elif line_aux.startswith('void'):
                            if 'noreturn' in line_aux:
                                write_lines.append('__weak '+ line_aux + ' { }')
                            else:
                                write_lines.append('__weak '+ line_aux + ' { return; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('unsigned int') or 'SVC_cx_call' in line_aux:
                            write_lines.append('__weak ' + line_aux + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('cx_err_t'):
                            write_lines.append('__weak ' + line_aux + ' { return 0x00000000; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bolos_err_t'):
                            write_lines.append('__weak ' + line_aux + ' { return 0xaa; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bolos_bool_t'):
                            write_lines.append('__weak ' + line_aux + ' { return 0xaa; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bool'):
                            write_lines.append('__weak ' + line_aux + ' { return true; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bolos_bool_t'):
                            write_lines.append('__weak ' + line_aux + ' { return 0xaa; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('bolos_task_status_t'):
                            write_lines.append('__weak ' + line_aux + ' { return 0x0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('uint8_t'):
                            write_lines.append('__weak ' + line_aux + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('try_context_t *'):
                            write_lines.append('__weak ' + line_aux + ' { return NULL; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('uint32_t'):
                            write_lines.append('__weak ' + line_aux + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('_Bool'):
                            write_lines.append('__weak ' + line_aux + ' { return true; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('unsigned char'):
                            write_lines.append('__weak ' + line_aux + ' { return \'M\'; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('unsigned short'):
                            write_lines.append('__weak ' + line_aux + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('int'):
                            write_lines.append('__weak ' + line_aux + ' { return 0; }')
                            i = skip_function(line, lines, i)
                        elif line_aux.startswith('const LANGUAGE_PACK *'):
                            write_lines.append('__weak ' + line_aux + ' { return NULL; }')
                            i = skip_function(line, lines, i)
                        else:
                            print(f"Skipped line [{i+1}]: {line_aux}")
                            i = skip_function(line, lines, i)

            case _:
                # Other not identified lines
                if '{' in line:
                    i = skip_function(line, lines, i)
                else:
                    print(f"Not identified line[{i+1}] = {line}")

        i += 1

    return write_lines

def main():

    if(len(sys.argv) > 2):
        functions_path = sys.argv[1]
        output_file = sys.argv[2]
        
        with open(functions_path, "r") as file:
            c_code = file.read()
            write_lines = gen_mocks(c_code)

        if write_lines:
            with open(output_file, "w") as file:
                for line in write_lines:
                    file.write(line + "\n")

    else:
        print("Usage: python3 gen_mock.py [c_functions_file.c]")

if __name__ == "__main__":
    main()