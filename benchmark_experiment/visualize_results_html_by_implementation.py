import json
import pandas as pd
import plotly.graph_objects as go
import argparse
import os

def parse_args():
    parser = argparse.ArgumentParser(description="Visualize per-implementation profiles.")
    parser.add_argument('-i', '--input', default='benchmark_results.json', help='Input JSON file')
    parser.add_argument('-o', '--output', default='implementation_profile.html', help='Output HTML file')
    return parser.parse_args()

def load_data(filepath):
    if not os.path.exists(filepath):
        print(f"File not found: {filepath}")
        exit(1)
    with open(filepath, 'r') as f:
        return json.load(f)

def flatten_json_to_df(data):
    """
    Flattens the nested results into a DataFrame using the custom 'Implementation' field.
    """
    rows = []
    results = data.get("results", {})

    # Ensure consistent file ordering (Alphabetical)
    sorted_filenames = sorted(results.keys())

    for file_idx, filename in enumerate(sorted_filenames):
        binaries = results[filename]
        for binary_alias, bin_data in binaries.items():
            measurements = bin_data.get("measurements", {})
            for bench_name, metrics in measurements.items():

                # --- YOUR SNIPPET LOGIC ---
                rows.append({
                    "FileIndex": file_idx + 1,
                    "Data File": filename,
                    "Binary": binary_alias,
                    "Benchmark": bench_name,
                    # Unique identifier: Benchmark Name + Binary Alias
                    "Implementation": f"{binary_alias}+{bench_name}",
                    "Speed": metrics.get("value_Mps", 0),
                    "Ratio": metrics.get("ratio", 0)
                })

    return pd.DataFrame(rows)

import html as _html
import json as _json

def create_dashboard(df, output_file):
    if df.empty:
        print("No data found.")
        return

    implementations = sorted(df["Implementation"].unique())
    fig = go.Figure()

    impl_trace_indices = {impl: [] for impl in implementations}
    trace_counter = 0

    for i, impl in enumerate(implementations):
        subset = df[df["Implementation"] == impl].sort_values("FileIndex")
        if subset.empty:
            continue

        fig.add_trace(go.Scatter(
            x=subset["FileIndex"],
            y=subset["Speed"],
            mode='lines+markers',
            name=impl,
            visible=False,  # <-- start hidden; checkboxes will control visibility
            marker=dict(size=6),
            line=dict(width=2),
            customdata=subset[["Data File", "Ratio", "Implementation"]],
            hovertemplate=(
                "<b>%{customdata[2]}</b><br>"
                "File: %{customdata[0]}<br>"
                "Speed: %{y:.2f} M/s<br>"
                "Ratio: %{customdata[1]:.2f}<extra></extra>"
            )
        ))

        impl_trace_indices[impl].append(trace_counter)
        trace_counter += 1

    fig.update_layout(
        title="Select implementations to compare",
        xaxis_title="Data Files (Index 1 to N)",
        yaxis_title="Speed (M/s)",
        template="plotly_dark",
        hovermode="closest",
        legend=dict(
            orientation="h",
            yanchor="top",
            y=-0.2,          # push below the x-axis area
            xanchor="center",
            x=0.5
        ),
        margin=dict(l=60, r=20, t=60, b=10)  # add bottom margin for legend
    )

    # --- Build custom HTML with checkboxes ---
    # Default: check first two (or fewer if not available)
    default_checked = set(implementations[:2])

    controls_html = "\n".join(
        f'''
        <label class="impl-item">
          <input type="checkbox" data-impl="{_html.escape(impl)}" {"checked" if impl in default_checked else ""}>
          <span>{_html.escape(impl)}</span>
        </label>
        '''
        for impl in implementations
    )

    # Produce only the plot div (no full HTML), then wrap it ourselves
    # plot_div = fig.to_html(full_html=False, include_plotlyjs="cdn", div_id="plot")
    plot_div = fig.to_html(
        full_html=False,
        include_plotlyjs="cdn",
        div_id="plot",
        config={"responsive": True},
        default_width="100%",
        default_height="100%"
    )
    mapping_json = json.dumps(impl_trace_indices)

    html_template = """<!doctype html>
    <html>
    <head>
      <meta charset="utf-8"/>
      <meta name="viewport" content="width=device-width, initial-scale=1"/>
      <title>Implementation Compare</title>
      <style>
        body {
          margin: 0;
          background: #111;
          color: #eee;
          height: 100%;
          font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial, sans-serif;
        }

        /* two-column layout */
        #wrap {
          display: flex;
          height: 100vh;
          width: 100vw;
        }

        #plot-wrap {
          flex: 1 1 auto;
          min-width: 0;          /* important so plot can shrink properly */
          min-height: 0;
          height: 100vh;
        }
        /* force plotly div to fill ONLY the left column */
        #plot {
          width: 100% !important;
          height: 100% !important;
        }

        #controls {
          flex: 0 0 300px;       /* sidebar width */
          height: 100vh;
          overflow: auto;
          padding: 12px;
          background: rgba(25,25,25,0.92);
          border-left: 1px solid rgba(255,255,255,0.18);
          box-shadow: -8px 0 24px rgba(0,0,0,0.35);
          font-size: 11px;
        }
        #controls .title {
          font-weight: 600;
          margin-bottom: 8px;
          font-size: 14px;
          opacity: 0.95;
        }
        .impl-item {
          display: flex;
          align-items: center;
          gap: 10px;
          padding: 6px 4px;
          border-radius: 8px;
          cursor: pointer;
          user-select: none;
        }
        .impl-item:hover {
          background: rgba(255,255,255,0.06);
        }
        .impl-item input {
          transform: scale(1.05);
        }
        .toolbar {
          display: flex;
          gap: 8px;
          margin-bottom: 8px;
        }
        .toolbar button {
          background: rgba(255,255,255,0.08);
          color: #eee;
          border: 1px solid rgba(255,255,255,0.14);
          border-radius: 8px;
          padding: 6px 10px;
          cursor: pointer;
        }
        .toolbar button:hover {
          background: rgba(255,255,255,0.12);
        }
      </style>
    </head>
    <body>
        <div id="wrap">
          <div id="plot-wrap">
            %%PLOT_DIV%%
          </div>

          <div id="controls">
            <div class="title">Compare implementations</div>
            <div class="toolbar">
              <button id="btn-none" type="button">Select none</button>
              <button id="btn-all" type="button">Select all</button>
              <button id="btn-top2" type="button">Select first 2</button>
            </div>
            %%CONTROLS%%
          </div>
        </div>

      <script>
        const implToTraces = %%MAPPING_JSON%%;
        const gd = document.getElementById("plot");

        function forceResize() {
            const wrap = document.getElementById("plot-wrap");
            if (!wrap || !gd) return;

            const w = wrap.clientWidth;
            const h = wrap.clientHeight;

            // This forces the figure to exactly fill the left column
            Plotly.relayout(gd, { width: w, height: h });
        }
        // after layout is applied
        requestAnimationFrame(forceResize);
        setTimeout(forceResize, 50);

        // when user resizes window
        window.addEventListener("resize", forceResize);


        function getCheckedImpls() {
          return [...document.querySelectorAll('#controls input[type="checkbox"]:checked')]
            .map(cb => cb.dataset.impl);
        }

        function updatePlot() {
          const checked = getCheckedImpls();
          const vis = new Array(gd.data.length).fill(false);

          checked.forEach(impl => {
            (implToTraces[impl] || []).forEach(idx => vis[idx] = true);
          });

          Plotly.restyle(gd, { visible: vis });

          const title = checked.length
            ? ("Implementations: " + checked.join("  vs  "))
            : "Select implementations to compare";
          Plotly.relayout(gd, { "title.text": title });
          setTimeout(resizePlotToContainer, 0);
        }

        document.querySelectorAll('#controls input[type="checkbox"]').forEach(cb => {
          cb.addEventListener('change', updatePlot);
        });

        document.getElementById("btn-none").addEventListener("click", () => {
          document.querySelectorAll('#controls input[type="checkbox"]').forEach(cb => cb.checked = false);
          updatePlot();
        });
        document.getElementById("btn-all").addEventListener("click", () => {
          document.querySelectorAll('#controls input[type="checkbox"]').forEach(cb => cb.checked = true);
          updatePlot();
        });
        document.getElementById("btn-top2").addEventListener("click", () => {
          const boxes = [...document.querySelectorAll('#controls input[type="checkbox"]')];
          boxes.forEach((cb, i) => cb.checked = (i < 2));
          updatePlot();
        });

        updatePlot();
      </script>
    </body>
    </html>
    """

    full_html = (html_template
        .replace("%%CONTROLS%%", controls_html)
        .replace("%%PLOT_DIV%%", plot_div)
        .replace("%%MAPPING_JSON%%", mapping_json)
    )


    print(f"Saving to {output_file}...")
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(full_html)
    print("Done.")


def main():
    args = parse_args()
    raw_data = load_data(args.input)
    df = flatten_json_to_df(raw_data)
    create_dashboard(df, args.output)

if __name__ == "__main__":
    main()
