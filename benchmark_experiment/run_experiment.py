import json
import os
import subprocess
import glob
import re
import time
import datetime
import sys
import argparse
import shlex
import compact_json
import platform
import itertools

# --- Constants ---
RECORD_SIZE_BYTES = 32

def parse_arguments():
    """Parses command line arguments."""
    parser = argparse.ArgumentParser(description="Run Google Benchmark suite using custom data files.")

    parser.add_argument(
        '-s', '--settings',
        default='settings.json',
        help='Path to the configuration JSON file (default: settings.json)'
    )

    parser.add_argument(
        '-o', '--output',
        default='benchmark_results.json',
        help='Path where the result JSON will be saved (default: benchmark_results.json)'
    )

    return parser.parse_args()

def load_relaxed_json(filepath):
    """
    Loads a JSON file, stripping C-style comments (//) and trailing commas.
    """
    if not os.path.exists(filepath):
        print(f"Error: Settings file '{filepath}' not found.")
        sys.exit(1)

    with open(filepath, 'r') as f:
        content = f.read()

    # 1. Remove comments (// ...)
    comment_pattern = re.compile(r'\s*//.*')
    content = re.sub(comment_pattern, '', content)

    # 2. Remove trailing commas
    trailing_comma_pattern = re.compile(r',(\s*[}\]])')
    content = re.sub(trailing_comma_pattern, r'\1', content)

    try:
        return json.loads(content)
    except json.JSONDecodeError as e:
        print(f"Error: Failed to parse JSON in '{filepath}'.\n{e}")
        sys.exit(1)

def find_data_files(data_dir):
    """Finds all files with .jacobi_data extension in the given directory."""
    data_dir = os.path.expanduser(data_dir)
    search_path = os.path.join(data_dir, "*.jacobi_data")
    files = glob.glob(search_path)
    if not files:
        print(f"Warning: No .jacobi_data files found in {data_dir}")
    return sorted(files)

def parse_benchmark_output(output_text):
    """
    Parses Google Benchmark stdout.
    1. Extracts items_per_second and converts to M/s.
    2. Calculates min/max for the whole set.
    3. Adds a 'ratio' field to each measurement (value / max).
    """
    temp_measurements = {}

    # Regex for parsing the specific benchmark output format
    regex = re.compile(r"^([\w\d_\/]+)\s+.+items_per_second=([0-9\.]+)([kMGT]?)\/s")

    # Multipliers to convert everything to M/s
    multipliers = { 'T': 1e6, 'G': 1e3, 'M': 1.0, 'k': 1e-3, '': 1e-6 }

    # 1. Parse raw values
    for line in output_text.splitlines():
        match = regex.search(line)
        if match:
            bench_name = match.group(1)
            raw_value = float(match.group(2))
            unit_suffix = match.group(3)

            factor = multipliers.get(unit_suffix, 0)
            final_value = raw_value * factor

            temp_measurements[bench_name] = final_value

    if not temp_measurements:
        return {}

    # 2. Calculate Stats
    values = list(temp_measurements.values())
    max_val = max(values)
    min_val = min(values)

    # 3. Build Rich Output Structure
    rich_measurements = {}
    for name, val in temp_measurements.items():
        ratio = (val / max_val) if max_val > 0 else 0.0
        rich_measurements[name] = {
            "value_Mps": val, # Explicitly denote unit in key
            "ratio": round(ratio, 4)
        }

    return {
        "summary": {
            "max_Mps": max_val,
            "min_Mps": min_val
        },
        "measurements": rich_measurements
    }

def calculate_range_env(data_file_path):
    """
    Calculates the B,E range string.
    1. Gets file size.
    2. Calculates N = size / 32.
    3. Cuts 5% from start and 5% from end.
    Returns string "B,E"
    """
    try:
        file_size = os.path.getsize(data_file_path)
        num_records = file_size // RECORD_SIZE_BYTES

        # Drop 5% from start, Keep until 95%
        # Integers B and E
        b_idx = int(num_records * 0.03)
        e_idx = int(num_records * 0.98)

        # Safety check for very small files
        if b_idx >= e_idx:
            b_idx = 0
            e_idx = num_records

        return f"{b_idx},{e_idx}"
    except OSError as e:
        print(f"  [!] Error calculating range for file {data_file_path}: {e}")
        return "0,0"

def get_system_info():
    """
    Gathers system information to mimic Google Benchmark output.
    Returns a dict containing 'run_on', 'caches', 'load_avg', etc.
    """
    info = {
        "host": platform.node(),
        "os": f"{platform.system()} {platform.release()} {platform.machine()}",
        "cpu_count": os.cpu_count() or 1,
        "cpu_model": platform.processor(),
    }

    # --- 1. Get Detailed CPU Model Name (Linux) ---
    if sys.platform.startswith('linux'):
        try:
            with open("/proc/cpuinfo", "r") as f:
                for line in f:
                    if "model name" in line:
                        # format: "model name : Intel(R) Core(TM)..."
                        info["cpu_model"] = line.split(':', 1)[1].strip()
                        break
        except Exception:
            pass

    # --- 2. Attempt to determine frequency for "Run on" string ---
    freq_mhz = 0
    try:
        if sys.platform.startswith('linux'):
            # Try getting max freq from sysfs (usually in kHz)
            # cpufreq/cpuinfo_max_freq is static max, scaling_max_freq is current policy max
            paths = [
                "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",
                "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
            ]
            for p in paths:
                if os.path.exists(p):
                    with open(p, 'r') as f:
                        # Value is in kHz, convert to MHz
                        freq_mhz = int(f.read().strip()) / 1000.0
                    break

            # Fallback to /proc/cpuinfo for frequency if sysfs failed
            if freq_mhz == 0:
                with open("/proc/cpuinfo", "r") as f:
                    for line in f:
                        if "cpu MHz" in line:
                            # format: "cpu MHz      : 2200.000"
                            parts = line.split(':')
                            if len(parts) > 1:
                                freq_mhz = float(parts[1].strip())
                            break
                        if "model name" in line:
                             info["cpu_model"] = line.split(':', 1)[1].strip()

    except Exception:
        pass # Fail silently on freq detection

    # Create the "Run on (...)" string
    # e.g. "Run on (8 X 4600 MHz CPU s)"
    if freq_mhz > 0:
        info["cpu_summary"] = f"{info['cpu_count']} X {freq_mhz:.0f} MHz CPU s"
    else:
        info["cpu_summary"] = f"{info['cpu_count']} X ? MHz CPU s"

    # --- Load Average ---
    if hasattr(os, "getloadavg"):
        info["load_avg"] = list(os.getloadavg())

    # --- Cache Info (Linux specific) ---
    if sys.platform.startswith('linux'):
        cache_dir = "/sys/devices/system/cpu/cpu0/cache"
        caches = []
        if os.path.exists(cache_dir):
            try:
                # Iterate index0, index1, etc.
                indices = sorted([d for d in os.listdir(cache_dir) if d.startswith("index")])
                for idx_dir in indices:
                    path = os.path.join(cache_dir, idx_dir)
                    with open(os.path.join(path, "size"), "r") as f: s = f.read().strip()
                    with open(os.path.join(path, "level"), "r") as f: l = f.read().strip()
                    with open(os.path.join(path, "type"), "r") as f: t = f.read().strip()

                    # Clean up type (Data, Instruction, Unified)
                    t_str = "Unified" if t.lower() == "unified" else t.capitalize()
                    caches.append(f"L{l} {t_str} {s}")
            except Exception:
                pass

        if caches:
            info["caches"] = caches

    return info

def run_single_benchmark(binary_path, data_file, wrapper, extra_args, extra_env_vars):
    """Runs a single binary against a single data file."""

    # 1. Base Environment
    env = os.environ.copy()

    # 2. Inject JACOBI variable
    env["JACOBI_BENCHMARK_EVENTS_FILE"] = data_file

    # 3. Inject per-binary custom environment variables
    if extra_env_vars:
        for k, v in extra_env_vars.items():
            env[k] = str(v) # Ensure values are strings

    # 4. Construct Command List
    cmd = []
    if wrapper:
        cmd.extend(shlex.split(wrapper))

    cmd.append(binary_path)
    cmd.extend(extra_args)

    # 5. Reconstruct the exact command string for the logs
    # We prepend the ENV variables explicitly for clarity in the log string

    # Start with Custom Env vars
    env_str_parts = []

    # Add the JACOBI var
    env_str_parts.append(f"JACOBI_BENCHMARK_EVENTS_FILE={shlex.quote(data_file)}")

    if extra_env_vars:
        for k, v in extra_env_vars.items():
            env_str_parts.append(f"{k}={shlex.quote(str(v))}")

    # Add the actual command
    cmd_base = " ".join(shlex.quote(arg) for arg in cmd)

    # Combine
    full_cmd_str = f"{' '.join(env_str_parts)} {cmd_base}"

    try:
        result = subprocess.run(
            cmd,
            env=env,
            capture_output=True,
            text=True,
            check=True
        )

        data = parse_benchmark_output(result.stdout)

        # Inject the command line into the summary
        if data and "summary" in data:
            data["summary"]["command_line"] = full_cmd_str

        return data

    except subprocess.CalledProcessError as e:
        print(f"  [!] Error running command: '{full_cmd_str}'\n      {e}")
        return {}
    except FileNotFoundError:
        print(f"  [!] Command or Binary not found. Path: {binary_path}")
        return {}

def save_results(filepath, settings, system_info, start_time, start_ts, results_tree):
    """Helper to save the JSON file with current timestamp stats."""

    current_time = datetime.datetime.now()
    current_ts = time.time()
    duration = current_ts - start_ts

    output_data = {
        "settings": settings,
        "system": system_info,
        "timestamps": {
            "start_iso": start_time.isoformat(),
            "end_iso": current_time.isoformat(),
            "duration_seconds": duration
        },
        "results": results_tree
    }

    formatter = compact_json.Formatter()
    formatter.indent_spaces = 4
    formatter.max_inline_complexity = 4
    formatter.max_inline_length=64

    with open(filepath, 'w') as f:
        f.write(formatter.serialize(output_data))
        os.fsync(f.fileno())

def main():
    args = parse_arguments()

    print("--- Starting Benchmark Suite ---")
    start_time = datetime.datetime.now()
    start_ts = time.time()

    # 1. Load Settings
    settings = load_relaxed_json(args.settings)

    # 2. Gather System Info
    system_info = get_system_info()
    print("System Info identified:", system_info)

    bin_dir = os.path.expanduser(settings.get("bin_dir", "."))
    data_dir = settings.get("data_dir", ".")
    binaries_config = settings.get("binaries", [])
    extra_args = settings.get("extra_args", [])
    wrapper = settings.get("wrapper", "")

    # 2. Find Data
    data_files = find_data_files(data_dir)

    results_tree = {}

    # 3. Execution Loop
    total_files = len(data_files)
    for i, d_file in enumerate(data_files):
        filename = os.path.basename(d_file)
        print(f"[{i+1}/{total_files}] Processing data: {filename}")

        results_tree[filename] = {}

        for bin_entry in binaries_config:
            if not isinstance(bin_entry, dict):
                print(f"  [!] Warning: Invalid binary config. Skipping.")
                continue

            bin_filename = bin_entry.get("file")
            bin_alias = bin_entry.get("alias")

            # 1. Determine Type
            bin_type = bin_entry.get("type", "full")

            # 2. Prepare Environment (Make a COPY of the base config env)
            current_run_env = bin_entry.get("env", {}).copy()

            if not bin_filename or not bin_alias:
                 print(f"  [!] Warning: Missing 'file' or 'alias' in: {bin_entry}")
                 continue

            binary_path = os.path.join(bin_dir, bin_filename)
            if os.name == 'nt' and not binary_path.endswith('.exe'):
                binary_path += '.exe'

            # 3. Handle specific types
            if bin_type == "range":
                range_str = calculate_range_env(d_file)
                current_run_env["JACOBI_BENCHMARK_EVENTS_RANGE"] = range_str
                print(f"  > Running: {bin_alias} [Range: {range_str}]")
            elif bin_type == "full":
                print(f"  > Running: {bin_alias}")
            else:
                print(f"  > Running: {bin_alias} (Unknown type '{bin_type}', defaulting to full)")

            # Pause: thermal cooldown...
            time.sleep(0.2)

            # --- Matrix or Standard Execution ---
            if "matrix" in settings:
                matrix = settings["matrix"]
                bsn_opts = matrix.get("bsn", [])
                plvl_opts = matrix.get("plvl", [])
                refIX_opts = matrix.get("refIX", [])

                combinations = list(itertools.product(bsn_opts, plvl_opts, refIX_opts))
                print(f"    - Matrix Mode: Executing {len(combinations)} combinations... ", end='', flush=True)

                aggregated_measurements = {}

                comman_lines = []
                max_mps = 0.0
                min_mps = 0.0
                max_name = "?"

                for (b, p, r) in combinations:
                    target_name = f"{b}_{p}_{r}"
                    # Strict filter for exact match
                    filter_arg = f"--benchmark_filter={target_name}"

                    # Combine args
                    run_args = extra_args + [filter_arg]

                    time.sleep(0.025)
                    # Execute
                    # Note: We do not sleep between matrix items to keep total time reasonable,
                    # but be aware of thermal throttling if count is high.
                    bench_data = run_single_benchmark(binary_path, d_file, wrapper, run_args, current_run_env)

                    comman_lines.append(bench_data["summary"]["command_line"])
                    current_meas = bench_data.get("measurements", {})

                    # Validation: Check 1 row and Name match
                    items = list(current_meas.items())
                    if len(items) != 1:
                        print(f"      [!] Warning: Matrix run for '{target_name}' returned {len(items)} rows. Expected 1.")

                    for m_name, m_data in items:
                        # Allow exact match
                        if m_name != target_name:
                             print(f"      [!] Warning: Output name '{m_name}' does not match target '{target_name}'")

                        # Add to aggregate
                        aggregated_measurements[m_name] = m_data
                        if max_mps < m_data["value_Mps"]:
                            max_mps = m_data["value_Mps"];
                            max_name = m_name

                        min_mps = min(min_mps, m_data["value_Mps"])

                # # Post-Matrix: Re-calculate stats (max, min, ratio) for the whole set
                # vals = [m['value_Mps'] for m in aggregated_measurements.values()]

                final_summary = {
                    "max_Mps": max_mps,
                    "max_at": max_name,
                    "min_Mps": min_mps,
                    "command_lines": comman_lines
                }
                print(f"MAX: {max_mps} Mps ({max_name})")

                # Update ratios based on new max
                for m in aggregated_measurements.values():
                    ratio = (m['value_Mps'] / max_mps) if max_mps > 0 else 0.0
                    m['ratio'] = round(ratio, 4)

                results_tree[filename][bin_alias] = {
                    "summary": final_summary,
                    "measurements": aggregated_measurements
                }
            else:
                # Standard Logic (Run once, parse all)
                bench_data = run_single_benchmark(binary_path, d_file, wrapper, extra_args, current_run_env)
                results_tree[filename][bin_alias] = bench_data

        print(f"  > Saving progress to {args.output}...")
        save_results(args.output, settings, system_info, start_time, start_ts, results_tree)

    print(f"\n--- Done! ---")
    print(f"Final results saved to: {os.path.abspath(args.output)}")

if __name__ == "__main__":
    main()
