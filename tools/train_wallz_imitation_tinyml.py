#!/usr/bin/env python3
"""Train an optional TinyML policy from Wall-Z Brain v0.4 PS4 demonstrations.

Input is /api/imitation/dataset output. The on-device v0.4 policy does NOT
require this script; it uses bounded k-NN imitation memory directly. This tool
is a reproducible path for compressing a larger demonstration history later.
"""
from __future__ import annotations

import argparse
import csv
from pathlib import Path

import numpy as np

ACTIONS = [
    "forward", "backward", "turn-left", "turn-right",
    "look-left", "look-right", "look-up", "look-down",
]
FEATURE_COLUMNS = [
    "distance_mm", "light_l", "light_r", "mic_l", "mic_r",
    "gx", "gy", "gz", "head_xy", "head_z",
    "vision_motion", "vision_x", "vision_y", "vision_familiarity",
]


def load_csv(path: Path):
    xs, ys = [], []
    with path.open(newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            action = row.get("action", "")
            if action not in ACTIONS:
                continue
            try:
                xs.append([float(row[c]) for c in FEATURE_COLUMNS])
            except (KeyError, ValueError):
                continue
            ys.append(ACTIONS.index(action))
    if len(xs) < 8:
        raise SystemExit("Need at least 8 valid PS4 demonstration rows")
    return np.asarray(xs, np.float32), np.asarray(ys, np.int32)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("dataset", type=Path)
    ap.add_argument("--out", type=Path, default=Path("wallz_imitation_int8.tflite"))
    ap.add_argument("--epochs", type=int, default=40)
    args = ap.parse_args()

    import tensorflow as tf

    x, y = load_csv(args.dataset)
    mean = x.mean(axis=0)
    scale = x.std(axis=0)
    scale[scale < 1e-6] = 1.0
    xn = (x - mean) / scale

    model = tf.keras.Sequential([
        tf.keras.layers.Input(shape=(len(FEATURE_COLUMNS),)),
        tf.keras.layers.Dense(24, activation="relu"),
        tf.keras.layers.Dense(16, activation="relu"),
        tf.keras.layers.Dense(len(ACTIONS), activation="softmax"),
    ])
    model.compile(optimizer="adam", loss="sparse_categorical_crossentropy", metrics=["accuracy"])
    model.fit(xn, y, epochs=args.epochs, batch_size=min(16, len(x)), verbose=2, validation_split=0.2 if len(x) >= 20 else 0.0)

    def representative():
        for row in xn[: min(len(xn), 128)]:
            yield [row.reshape(1, -1).astype(np.float32)]

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    blob = converter.convert()
    args.out.write_bytes(blob)
    args.out.with_suffix(args.out.suffix + ".labels.txt").write_text("\n".join(ACTIONS) + "\n", encoding="utf-8")
    args.out.with_suffix(args.out.suffix + ".norm.csv").write_text(
        "feature,mean,scale\n" + "".join(f"{n},{m:.9g},{s:.9g}\n" for n, m, s in zip(FEATURE_COLUMNS, mean, scale)),
        encoding="utf-8",
    )
    print(f"wrote {args.out} ({len(blob)} bytes) from {len(x)} demonstrations")


if __name__ == "__main__":
    main()
