import os
import re

def extract_non_static_functions(c_file):
    function_pattern = re.compile(r'^\s*([a-zA-Z_][\w\s\*#_\(\)]*)\s+([a-zA-Z_]\w*)\s*\(([^)]*)\)\s*\{', re.MULTILINE)

    static_pattern = re.compile(r'^\s*static\s+')

    forward_declarations = []

    with open(c_file, 'r') as f:
        content = f.read()

    functions = function_pattern.findall(content)

    for return_type, function_name, arguments in functions:
        func_start = content.find(function_name)
        before_func = content[:func_start]
        static_match = static_pattern.search(before_func.splitlines()[-1])

        if static_match:
            continue

        forward_decl = f"{return_type.strip()} {function_name}({arguments.strip()});"
        forward_declarations.append(forward_decl)

    return forward_declarations


def write_forward_declarations(forward_declarations, output_file):
    with open(output_file, 'w') as f:
        for decl in forward_declarations:
            f.write(decl + "\n")


def find_c_files_in_directory(root_dir):
    c_files = []
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.endswith(".c"):
                c_files.append(os.path.join(dirpath, filename))
    return c_files


if __name__ == '__main__':
    root_directory = os.getcwd()

    c_files = find_c_files_in_directory("/src")

    output_file = 'util/forward_declarations.h'

    all_declarations = []

    for c_file in c_files:
        forward_declarations = extract_non_static_functions(c_file)
        #print(forward_declarations)
        all_declarations.extend(forward_declarations)

    if all_declarations:
        write_forward_declarations(all_declarations, output_file)
        print(f"forward declarations for all C files written to {output_file}")
    else:
        print("no non-static functions found.")
