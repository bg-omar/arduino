#!/usr/bin/env python3
"""Train a small Wall-Z semantic CNN and export fully-int8 TFLite.

Accepted input:
1) Brain v0.3 downloaded CSV: each line is `label,<600 hex chars>`.
2) NPZ with keys x [N,15,20,1], y [N], labels [K].

The script uses only real labelled Wall-Z samples. It never synthesizes fake
person/object training data.
"""
import argparse
import pathlib
import numpy as np

H, W = 15, 20


def load_csv(path):
    xs, names = [], []
    for no, raw in enumerate(pathlib.Path(path).read_text(encoding="utf-8").splitlines(), 1):
        raw = raw.strip()
        if not raw:
            continue
        try:
            label, payload = raw.split(",", 1)
        except ValueError:
            raise SystemExit(f"line {no}: expected label,hex")
        if len(payload) != H * W * 2:
            raise SystemExit(f"line {no}: expected {H*W*2} hex chars, got {len(payload)}")
        try:
            grid = np.frombuffer(bytes.fromhex(payload), dtype=np.uint8).reshape(H, W, 1)
        except ValueError as e:
            raise SystemExit(f"line {no}: invalid hex: {e}")
        xs.append(grid)
        names.append(label)
    if not xs:
        raise SystemExit("dataset is empty")
    labels = sorted(set(names))
    lut = {name: i for i, name in enumerate(labels)}
    y = np.asarray([lut[n] for n in names], dtype=np.int32)
    return np.stack(xs), y, np.asarray(labels)


def load_dataset(path):
    if str(path).lower().endswith(".npz"):
        d = np.load(path, allow_pickle=True)
        x = d["x"].astype(np.float32)
        y = d["y"].astype(np.int32)
        labels = d["labels"]
        if x.ndim == 3:
            x = x[..., None]
        return x, y, labels
    return load_csv(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dataset", help="wallz_vision_dataset.csv or compatible NPZ")
    ap.add_argument("--out", default="wallz_visual_int8.tflite")
    ap.add_argument("--epochs", type=int, default=30)
    args = ap.parse_args()

    x, y, labels = load_dataset(args.dataset)
    if x.shape[1:] != (H, W, 1):
        raise SystemExit(f"expected [N,{H},{W},1], got {x.shape}")
    if len(labels) < 2:
        raise SystemExit("need at least 2 different labels for semantic training")

    # Import TensorFlow only after dataset validation so CSV problems remain easy to diagnose.
    import tensorflow as tf

    x = x.astype(np.float32) / 255.0
    model = tf.keras.Sequential([
        tf.keras.layers.Input((H, W, 1)),
        tf.keras.layers.Conv2D(8, 3, padding="same", activation="relu"),
        tf.keras.layers.MaxPool2D(2),
        tf.keras.layers.Conv2D(12, 3, padding="same", activation="relu"),
        tf.keras.layers.GlobalAveragePooling2D(),
        tf.keras.layers.Dense(len(labels), activation="softmax"),
    ])
    model.compile(optimizer="adam", loss="sparse_categorical_crossentropy", metrics=["accuracy"])
    model.fit(x, y, epochs=args.epochs, validation_split=0.2, shuffle=True)

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]

    def representative():
        for i in range(min(len(x), 200)):
            yield [x[i:i + 1]]

    converter.representative_dataset = representative
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    model_bytes = converter.convert()

    pathlib.Path(args.out).write_bytes(model_bytes)
    pathlib.Path(args.out + ".labels.txt").write_text("\n".join(map(str, labels)), encoding="utf-8")
    print(f"samples={len(x)} classes={len(labels)} labels={list(map(str,labels))}")
    print(f"wrote {args.out}: {len(model_bytes)} bytes")


if __name__ == "__main__":
    main()
