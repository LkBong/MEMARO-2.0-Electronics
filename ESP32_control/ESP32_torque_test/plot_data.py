import argparse
import os
import sys

import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots

K_T_DEFAULT = 0.00847  # Nm/A (8.47 mNm/A)


def resolve_path(csv_arg: str) -> str:
    if os.path.isfile(csv_arg):
        return csv_arg
    data_path = os.path.join(os.path.dirname(__file__), "data", os.path.basename(csv_arg))
    if os.path.isfile(data_path):
        return data_path
    sys.exit(f"Error: cannot find '{csv_arg}' — tried as-is and in data/")


def plot(csv_path: str, kt: float) -> None:
    df = pd.read_csv(csv_path)
    df["elapsed_s"] = df["elapsed_ms"] / 1000.0
    df["torque_nm"] = df["current_a"] * kt

    filename = os.path.basename(csv_path)
    title = f"{filename}  |  Kₜ = {kt*1000:.3f} mNm/A"

    fig = make_subplots(
        rows=3, cols=1,
        shared_xaxes=True,
        vertical_spacing=0.06,
        subplot_titles=("Linear Speed", "Current", "Torque"),
    )

    common = dict(mode="lines", line=dict(width=1.5))

    fig.add_trace(
        go.Scatter(x=df["elapsed_s"], y=df["linear-speed_ms-1"],
                   name="Speed (ms-1)", line=dict(color="#1f77b4", **{k: v for k, v in common["line"].items() if k != "color"}),
                   mode="lines", hovertemplate="%{y:.4f} ms-1<extra></extra>"),
        row=1, col=1,
    )
    fig.add_trace(
        go.Scatter(x=df["elapsed_s"], y=df["current_a"],
                   name="Current (A)", line=dict(color="#ff7f0e", width=1.5),
                   mode="lines", hovertemplate="%{y:.3f} A<extra></extra>"),
        row=2, col=1,
    )
    fig.add_trace(
        go.Scatter(x=df["elapsed_s"], y=df["torque_nm"] * 1000,
                   name="Torque (mNm)", line=dict(color="#2ca02c", width=1.5),
                   mode="lines", hovertemplate="%{y:.2f} mNm<extra></extra>"),
        row=3, col=1,
    )

    fig.update_yaxes(title_text="m/s", row=1, col=1)
    fig.update_yaxes(title_text="A", row=2, col=1)
    fig.update_yaxes(title_text="mNm", row=3, col=1)
    fig.update_xaxes(title_text="Time (s)", row=3, col=1,
                     rangeslider=dict(visible=True, thickness=0.04))

    fig.update_layout(
        title=title,
        hovermode="x unified",
        height=750,
        legend=dict(orientation="h", y=-0.12),
        margin=dict(t=60, b=80),
    )

    fig.show()


def main() -> None:
    parser = argparse.ArgumentParser(description="Plot torque test CSV data.")
    parser.add_argument("csv_file", help="CSV filename or path (searches data/ automatically)")
    parser.add_argument("--kt", type=float, default=K_T_DEFAULT,
                        help=f"Torque constant in Nm/A (default: {K_T_DEFAULT})")
    args = parser.parse_args()

    csv_path = resolve_path(args.csv_file)
    plot(csv_path, args.kt)


if __name__ == "__main__":
    main()
