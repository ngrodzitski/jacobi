import csv
import json
import argparse
import sys
import sqlite3
import os
import re
import copy
import compact_json

def is_valid_table_name(name):
    """Ensures the table name is SQL-safe (letters, numbers, underscores only)."""
    return re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', name) is not None

def substitute_pattern(str, number, compiled_pattern):
    formatted_number = f"{number:03d}"
    return compiled_pattern.sub(formatted_number, str)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', required=True, help='Input JSON file')
    parser.add_argument('-o', '--output', required=True, help='Output JSON file')
    parser.add_argument('-x', '--regex', required=True, help='Pattern to replace in filenames')
    args = parser.parse_args()

    try:
        compiled_regex = re.compile(args.regex)
    except re.error as e:
        sys.exit(f"Error: Invalid regular expression pattern - {e}")

    # Load JSON
    try:
        with open(args.input, 'r') as f:
            benchmark = json.load(f)
    except Exception as e:
        sys.exit(f"Failed to load JSON file: {e}")

    counter=0

    results = benchmark["results"]
    new_reults = {}
    for jacobi_file, details in results.items():
        new_details = {}
        for approach, approach_details in details.items():
            new_approach_details = copy.deepcopy(approach_details)
            new_approach_details["summary"].pop("command_lines", None)
            new_approach_details["summary"].pop("command_line", None)
            new_details[approach] = new_approach_details

        counter = counter + 1
        censored_name = substitute_pattern(jacobi_file, counter, compiled_regex)
        print(f"{censored_name}\t{jacobi_file}")
        new_reults[censored_name] = new_details

    benchmark["results"] = new_reults

    formatter = compact_json.Formatter()
    formatter.indent_spaces = 4
    formatter.max_inline_complexity = 4
    formatter.max_inline_length=64

    with open(args.output, 'w') as f:
        f.write(formatter.serialize(benchmark))
        os.fsync(f.fileno())

if __name__ == "__main__":
    main()
