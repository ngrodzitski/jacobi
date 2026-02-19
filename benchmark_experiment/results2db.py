import csv
import json
import argparse
import sys
import sqlite3
import os
import re

def is_valid_table_name(name):
    """Ensures the table name is SQL-safe (letters, numbers, underscores only)."""
    return re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', name) is not None

def validate_and_extract(data):
    """Validates the nested structure and yields rows of data."""
    if "results" not in data:
        sys.exit("Error: Missing top-level 'results' field.")

    results = data["results"]
    if not isinstance(results, dict):
        sys.exit("Error: 'results' must be an object.")

    # Level 1: Jacobi Data
    for jacobi_key, storage_map in results.items():
        if not isinstance(storage_map, dict):
            sys.exit(f"Error: results['{jacobi_key}'] must be an object.")

        # Level 2: Orders Table Storage
        for storage_key, storage_val in storage_map.items():
            if not isinstance(storage_val, dict) or "measurements" not in storage_val:
                sys.exit(f"Error: '{storage_key}' must be an object containing 'measurements'.")

            measurements = storage_val["measurements"]
            if not isinstance(measurements, dict):
                sys.exit(f"Error: 'measurements' under '{storage_key}' must be an object.")

            # Level 3: Components
            for component_key, component_val in measurements.items():
                if not isinstance(component_val, dict) or "value_Mps" not in component_val:
                    sys.exit(f"Error: Component '{component_key}' must contain 'value_Mps'.")

                # Yield the specific data for this row
                yield {
                    "jacobi_data": jacobi_key,
                    "orders_table_storage": storage_key,
                    "components": component_key,
                    "mps": component_val["value_Mps"]
                }

def write_csv(filename, rows, fieldnames):
    new_file = not os.path.exists(filename)
    with open(filename, mode='a', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if new_file or os.path.getsize(filename) == 0:
            writer.writeheader()
        writer.writerows(rows)
    print(f"Logged {len(rows)} rows to CSV: {filename}")

def write_sqlite(filename, rows, table_name):
    if not is_valid_table_name(table_name):
        sys.exit(f"Error: Invalid table name '{table_name}'. Use only letters, numbers, and underscores.")

    conn = sqlite3.connect(filename)
    cursor = conn.cursor()

    # 1. Create Table dynamically using the provided table_name
    cursor.execute(f'''
        CREATE TABLE IF NOT EXISTS {table_name} (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            git_commit TEXT,
            compiler TEXT,
            jacobi_data TEXT,
            orders_table_storage TEXT,
            components TEXT,
            mps REAL
        )
    ''')

    # 2. Create Indexes
    cursor.execute(f'''
        CREATE INDEX IF NOT EXISTS idx_{table_name}_comp_storage
            ON {table_name} (compiler, orders_table_storage)
    ''')
    cursor.execute(f'''
        CREATE INDEX IF NOT EXISTS idx_{table_name}_comp_storage_components
            ON {table_name} (compiler, orders_table_storage, components)
    ''')
    cursor.execute(f'''
        CREATE INDEX IF NOT EXISTS idx_{table_name}_book_unit
            ON {table_name} (orders_table_storage, components)
    ''')
    cursor.execute(f'''
        CREATE INDEX IF NOT EXISTS idx_{table_name}_file_unit
            ON {table_name} (jacobi_data)
    ''')

    # 3. Insert Data
    # We convert the list of dicts to a list of tuples for executemany
    data_tuples = [
        (r['git_commit'], r['compiler'], r['jacobi_data'],
         r['orders_table_storage'], r['components'], r['mps'])
        for r in rows
    ]

    cursor.executemany(f'''
        INSERT INTO {table_name} (git_commit, compiler, jacobi_data, orders_table_storage, components, mps)
        VALUES (?, ?, ?, ?, ?, ?)
    ''', data_tuples)

    conn.commit()
    conn.close()
    print(f"Logged {len(rows)} rows to SQLite: {filename}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--json_path", required=True)
    parser.add_argument("--output", required=True, help="Filename ending in .csv or .db/.sqlite")
    parser.add_argument("--table", default="measurements", help="Table name for SQLite output (default: measurements)")
    args = parser.parse_args()

    # Load JSON
    try:
        with open(args.json_path, 'r') as f:
            raw_data = json.load(f)
    except Exception as e:
        sys.exit(f"Failed to load JSON file: {e}")

    # Prepare Data Buffer
    all_rows = []
    for entry in validate_and_extract(raw_data):
        full_row = {
            "git_commit": args.commit,
            "compiler": args.compiler,
            **entry
        }
        all_rows.append(full_row)

    if not all_rows:
        print("No valid data found in JSON to write.")
        sys.exit(0)

    # Detect Format based on extension
    if args.output.endswith('.csv'):
        fieldnames = ["git_commit", "compiler", "jacobi_data", "orders_table_storage", "components", "mps"]
        write_csv(args.output, all_rows, fieldnames)
    elif args.output.endswith(('.db', '.sqlite', '.sqlite3')):
        write_sqlite(args.output, all_rows, args.table)
    else:
        sys.exit("Error: Output file must end with .csv, .db, or .sqlite")

if __name__ == "__main__":
    main()
