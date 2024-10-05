import os
import re

def extract_enums_from_file(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

    enum_regex = r"(?:typedef\s+)?enum\s*\{(.*?)\}\s*(\w+);"
    matches = re.findall(enum_regex, content, re.DOTALL)
    
    enums = {}
    for match in matches:
        enum_block, enum_name = match
        enums[enum_name] = enum_block.strip()
    
    return enums

def generate_enum_map_file(header_name, enums):
    with open(f"{header_name}.h", 'w') as file:
        file.write("#pragma once\n\n")
        for enum_name, enum_block in enums.items():
            enum_values = [line.split('=')[0].strip().replace(',', '') 
                           for line in enum_block.splitlines() 
                           if line.strip() and not line.strip().startswith("//")]
            
            file.write(f"// mapping for {enum_name}\n")
            file.write(f"static const char* {enum_name}_to_string[] = {{\n")
            for value in enum_values:
                file.write(f"    \"{value}\",\n")
            file.write("};\n\n")

def find_c_and_h_files(folder):
    c_h_files = []
    for root, dirs, files in os.walk(folder):
        for file in files:
            if file.endswith('.c') or file.endswith('.h'):
                c_h_files.append(os.path.join(root, file))
    return c_h_files

def process_all_enums_in_folder(folder, output_header_name):
    all_enums = {}
    files = find_c_and_h_files(folder)
    
    for file_path in files:
        enums_in_file = extract_enums_from_file(file_path)
        all_enums.update(enums_in_file)
    
    if all_enums:
        generate_enum_map_file(output_header_name, all_enums)
        print(f"header file '{output_header_name}.h' has been generated successfully.")
    else:
        print(f"no enums found in the folder '{folder}'.")

folder_path = "src"
output_header_name = "util/enum_map"

process_all_enums_in_folder(folder_path, output_header_name)
