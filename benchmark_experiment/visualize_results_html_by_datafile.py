import json
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import argparse
import os

def parse_args():
    parser = argparse.ArgumentParser(description="Visualize benchmark results.")
    parser.add_argument('-i', '--input', default='benchmark_results.json', help='Input JSON file')
    parser.add_argument('-o', '--output', default='benchmark_report.html', help='Output HTML file')
    return parser.parse_args()

def load_data(filepath):
    if not os.path.exists(filepath):
        print(f"File not found: {filepath}")
        exit(1)
    with open(filepath, 'r') as f:
        return json.load(f)

def flatten_json_to_df(data):
    """
    Converts the nested JSON results into a flat Pandas DataFrame.
    """
    rows = []

    # Structure: results -> filename -> binary_alias -> measurements
    results = data.get("results", {})

    for filename, binaries in results.items():
        for binary_alias, bin_data in binaries.items():
            measurements = bin_data.get("measurements", {})

            for bench_name, metrics in measurements.items():
                rows.append({
                    "Data File": filename,
                    "Binary": binary_alias,
                    "Benchmark": bench_name,
                    "Speed (M/s)": metrics.get("value_Mps", 0),
                    "Ratio": metrics.get("ratio", 0)
                })

    return pd.DataFrame(rows)

def create_dashboard(df, output_file):
    """
    Creates an interactive HTML file with a dropdown to switch between Data Files.
    """
    if df.empty:
        print("No data found to visualize.")
        return

    # Get list of unique files for the dropdown
    data_files = df["Data File"].unique()

    # Initialize the figure
    fig = go.Figure()

    # --- 1. Create Traces ---
    # We create a set of bars for EVERY data file, but initially hide all except the first one.

    # We need to track how many traces belong to each file to manage visibility
    traces_per_file = []

    for i, filename in enumerate(data_files):
        file_df = df[df["Data File"] == filename]

        # We need to pivot so we have columns for each binary to group them easily
        # However, Plotly Graph Objects works best by adding traces manually for grouping
        binaries = file_df["Binary"].unique()

        count = 0
        for binary in binaries:
            subset = file_df[file_df["Binary"] == binary]

            visible = (i == 0) # Only make the first file visible by default

            fig.add_trace(go.Bar(
                x=subset["Benchmark"],
                y=subset["Speed (M/s)"],
                name=binary,
                visible=visible,
                customdata=subset[["Ratio"]],
                hovertemplate=(
                    "<b>%{x}</b><br>" +
                    "Binary: " + binary + "<br>" +
                    "Speed: %{y:.2f} M/s<br>" +
                    "Ratio: %{customdata[0]:.2f}<extra></extra>"
                )
            ))
            count += 1
        traces_per_file.append(count)

    # --- 2. Create Dropdown Buttons ---
    buttons = []
    current_idx = 0

    for i, filename in enumerate(data_files):
        # Create a visibility array [False, False, ..., True, True, ..., False]
        # visible_status needs to match the total number of traces in the figure
        visible_status = [False] * len(fig.data)

        # Calculate the range of traces belonging to this file
        num_traces = traces_per_file[i]
        for j in range(num_traces):
            visible_status[current_idx + j] = True

        buttons.append(dict(
            label=filename,
            method="update",
            args=[{"visible": visible_status},
                  {"title": f"Benchmark Results: {filename}"}]
        ))

        current_idx += num_traces

    # --- 3. Layout Configuration ---
    fig.update_layout(
        title=f"Benchmark Results: {data_files[0]}",
        updatemenus=[{
            "active": 0,
            "buttons": buttons,
            "x": 1.15,
            "y": 1.1,
            "xanchor": "right",
            "yanchor": "top",
            "bgcolor": "#333333",   # Dark grey background for the menu
            "bordercolor": "#888888",
            "font": {"color": "#FFFFFF", "shadow": "auto"}
        }],
        barmode='group', # Side-by-side bars
        xaxis_tickangle=-45,
        yaxis_title="Items Per Second (M/s)",
        template="plotly_dark", # Looks cool, easier on eyes
        margin=dict(b=150) # Extra space for long benchmark names
    )

    # Save
    print(f"Generating report: {output_file}...")
    fig.write_html(output_file)
    print("Done! Open the HTML file in your browser.")

def main():
    args = parse_args()
    raw_data = load_data(args.input)
    df = flatten_json_to_df(raw_data)
    create_dashboard(df, args.output)

if __name__ == "__main__":
    main()
