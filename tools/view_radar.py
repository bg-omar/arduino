#!/usr/bin/env python3
"""3D scatter plot of Wall-Z radar SCAN*.CSV point clouds."""

import argparse
import csv
import sys


def load_points(path: str):
    points = []
    with open(path, newline="") as f:
        for row in csv.reader(f):
            if not row or row[0].startswith("#"):
                continue
            if len(row) < 6:
                continue
            try:
                x, y, z = float(row[3]), float(row[4]), float(row[5])
            except ValueError:
                continue
            points.append((x, y, z))
    return points


def main():
    parser = argparse.ArgumentParser(description="View Wall-Z radar CSV in 3D")
    parser.add_argument("csv", help="Path to SCAN###.CSV")
    args = parser.parse_args()

    points = load_points(args.csv)
    if not points:
        print("No valid points in file", file=sys.stderr)
        return 1

    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib required: pip install matplotlib", file=sys.stderr)
        return 1

    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    zs = [p[2] for p in points]

    fig = plt.figure(figsize=(8, 6))
    ax = fig.add_subplot(111, projection="3d")
    ax.scatter(xs, ys, zs, c=zs, cmap="viridis", s=8, depthshade=True)
    ax.set_xlabel("X right (cm)")
    ax.set_ylabel("Y forward (cm)")
    ax.set_zlabel("Z up (cm)")
    ax.set_title(f"Radar scan: {len(points)} points")
    plt.tight_layout()
    plt.show()
    return 0


if __name__ == "__main__":
    sys.exit(main())
