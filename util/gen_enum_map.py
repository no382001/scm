import re

def extract_enum_from_file(file_path, enum_name):
    with open(file_path, 'r') as file:
        content = file.read()
    
    enum_regex = rf"typedef enum \{{(.*?)\}} {enum_name};"
    match = re.search(enum_regex, content, re.DOTALL)
    
    if match:
        return match.group(1)
    else:
        raise ValueError(f"enum '{enum_name}' not found in the file.")

def generate_enum_map(header_name, enum_name, enum_block):
    enum_values = [line.split('=')[0].strip().replace(',', '') for line in enum_block.splitlines() if line.strip() and not line.strip().startswith("//")]
    
    with open(f"{header_name}.h", 'w') as file:
        file.write("#pragma once\n")
        file.write(f"static const char* {enum_name}_to_string[] = {{\n")
        for value in enum_values:
            file.write(f"    \"{value}\",\n")

c_file_path = "src/tinyscheme.h"

enum_name = "ERROR_T"

try:
    enum_block = extract_enum_from_file(c_file_path, enum_name)
    generate_enum_map("util/error_map", enum_name, enum_block)
    print(f"header file 'error_map.h' has been generated successfully.")
except ValueError as e:
    print(e)
