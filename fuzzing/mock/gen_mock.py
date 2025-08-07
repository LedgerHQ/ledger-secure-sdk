import re
import sys

NUM_WRITTEN_MOCKS = 0
NUM_SKIPPED_MOCKS = 0

RETURN_MOCKS = {
    "void": ["{ __builtin_unreachable(); }", "{ return; }"],
    "unsigned int": "{ return 0; }",
    "SVC_cx_call": "{ return 0; }",
    "cx_err_t": "{ return 0x00000000; }",
    "bolos_err_t": "{ return 0xaa; }",
    "bool": "{ return true; }",
    "bolos_bool_t": "{ return 0xaa; }",
    "bolos_task_status_t": "{ return 0x0; }",
    "uint8_t": "{ return 0; }",
    "uint8_t*": "{ return NULL; }",
    "try_context_t*": "{ return NULL; }",
    "uint32_t": "{ return 0; }",
    "_Bool": "{ return true; }",
    "unsigned char": "{ return 'M'; }",
    "unsigned short": "{ return 0; }",
    "int": "{ return 0; }",
    "const LANGUAGE_PACK*": "{ return NULL; }"
}

def mark_params_unused(signature: str) -> str:
    global NUM_WRITTEN_MOCKS
    if '(' not in signature or ')' not in signature:
        return signature

    prefix, param_str = signature.split('(', 1)
    param_str, suffix = param_str.rsplit(')', 1)
    params = [p.strip() for p in param_str.split(',')]
    NUM_WRITTEN_MOCKS += 1

    def modify(p):
        if not p or p == 'void' or '__attribute__' in p:
            return p
        parts = p.rsplit(' ', 1)
        return f"{parts[0]} {parts[1]} __attribute__((unused))" if len(parts) == 2 else f"{p} __attribute__((unused))"

    return f"{prefix}({', '.join(modify(p) for p in params)}){suffix}"

def skip_function_body(lines, i):
    brace_count = lines[i].count('{') - lines[i].count('}')
    i += 1
    while brace_count > 0 and i < len(lines):
        brace_count += lines[i].count('{') - lines[i].count('}')
        i += 1
    return i - 1

def extract_signature(lines, i):
    sig = lines[i].strip()
    while ')' not in sig and i < len(lines) - 1:
        i += 1
        sig += ' ' + lines[i].strip()
    return sig, i

def generate_mock(signature):
    ret_type, func_name = signature.split('(', 1)[0].strip().rsplit(' ', 1)
    if func_name.startswith('*'):
        ret_type = ret_type + '*'

    mock = RETURN_MOCKS.get(ret_type)
    if isinstance(mock, list):  # void case
        return mock[0] if 'noreturn' in signature else mock[1]
    return mock

def clean_duplicate_attributes(signature: str) -> str:
    return signature.replace("__attribute((weak))", "").strip()

def gen_mocks(c_code):
    global NUM_SKIPPED_MOCKS
    lines = c_code.splitlines()
    mocks, skipped = [], []
    i = 0

    while i < len(lines):
        lines[i] = clean_duplicate_attributes(lines[i])
        line = lines[i]

        if line.strip().startswith('#') or not line.strip():
            mocks.append(line + '\n')
        elif re.match(r'^\s*[\w\s\*\_]+[*\s]+\w+\s*\(.*', line):
            if ';' in line:
                mocks.append(line + '\n')
            else:
                signature, i = extract_signature(lines, i)
                mock_body = generate_mock(signature)

                if mock_body:
                    mocks.append(f"__attribute__((weak)) {mark_params_unused(signature)} {mock_body}\n")
                    i = skip_function_body(lines, i)
                else:
                    skipped.append(f"Skipped line [{i+1}]: {signature}\n")
                    i = skip_function_body(lines, i)
        elif '{' in line:
            i = skip_function_body(lines, i)
        else:
            NUM_SKIPPED_MOCKS += 1
            skipped.append(f"Ignoring line [{i+1}]: {line}\n")

        i += 1

    return mocks, skipped

def main():
    WARNING, ENDC = '\033[93m', '\033[0m'

    if len(sys.argv) < 3:
        print("Usage: python3 gen_mock.py [path/input.c] [path/output.c]")
        return

    input_path, output_path = sys.argv[1], sys.argv[2]
    skipped_path = output_path.rsplit('.', 1)[0] + '_skipped.txt'

    with open(input_path, 'r') as f:
        c_code = f.read()

    mocks, skipped = gen_mocks(c_code)

    with open(output_path, 'w') as f:
        f.writelines(mocks)

    if skipped:
        with open(skipped_path, 'w') as f:
            f.writelines(skipped)

    print(f"{WARNING}{NUM_WRITTEN_MOCKS} mocks generated at {output_path}{ENDC}")
    print(f"{WARNING}{NUM_SKIPPED_MOCKS} lines skipped, written at {skipped_path}{ENDC}")

if __name__ == "__main__":
    main()
