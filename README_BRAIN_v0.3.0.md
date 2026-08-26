# Wall-Z Brain v0.3.0 — teach-able vision memory

Brain v0.3 is based directly on `Wall-Z_source_v0.2-merged` and preserves the v0.2 motor, servo, PS4, radar and safety behavior.

## New in v0.3

The fisheye ESP32-CAM keeps the low-latency path:

```text
160x120 grayscale
    -> 20x15 local grid
    -> motion + attention x/y at ~20 Hz
```

It now also sends a compact 20x15 grayscale snapshot to the onboard ESP32-S3 at 2 Hz. The S3 can learn up to 8 named visual concepts and stores them persistently in NVS.

Examples of labels:

```text
person
ball
door
Kato
charger
```

Labels may contain letters, digits, `-` and `_` and are at most 15 characters.

## Teaching Wall-Z

1. Start RA4M1, onboard S3 and the fisheye ESP32-CAM.
2. Open the Brain web page.
3. Put the object/person roughly in the fisheye view.
4. Type a label and click **Teach current view**.
5. Repeat 3–8 times from slightly different positions/poses.
6. The dashboard shows the recognized concept and familiarity score (0..1000).
7. `+ reward` / `- reward` also updates the learned value associated with the currently recognized concept.

Recognition is intentionally transparent: the 20x15 image is brightness-normalized, stored as an on-device prototype, and matched with ±1 grid-cell translation tolerance. This gives immediate one-shot learning instead of pretending that a generic pretrained model already knows your particular fisheye, room and objects.

## UART protocol

Existing fast telemetry remains unchanged:

```text
V,ms,motion,x,y,brightness,contrast,fps_x10,flags
```

New learning snapshot:

```text
G,ms,seq,mean,contrast,<600 hex characters>
```

New S3 -> fisheye commands:

```text
C,GRID,1..5
C,SNAPSHOT
```

At 2 snapshots/s, the grid payload is roughly 1.25 kB/s including ASCII encoding, far below the 230400-baud UART capacity. Full image frames are never sent over this link.

## Safety / authority

Unchanged:

```text
PS4/manual
    > RA4M1 safety
    > existing robot modes
    > Brain
    > fisheye perception
```

The fisheye cannot command motors. Brain commands still pass through RA4M1 heartbeat timeout, manual override, sonar forward gate, 350 ms maximum movement duration and robot-mode lockouts.

The merged v0.2 fixes remain authoritative:

- `B,BRAKE` stops motion without disarming autonomy.
- radar maximum remains 500 cm.
- sonar no-echo within the 500 cm search range remains interpreted as clear for the Brain forward gate.
- `brain_link::poll()`/`tick()` behavior is not regressed.
- `Motor::Car_creepBack()` remains present.

## TensorFlow / TinyML

TensorFlow is useful here, but semantic recognition needs real labelled examples from Wall-Z's fisheye. Brain v0.3 therefore does not ship a fake `person` model trained on synthetic shapes.

`tools/train_wallz_visual_tinyml.py` is a real TensorFlow/Keras training and fully-int8 TFLite export path for 20x15 labelled grids. It is intended for the next semantic backend after real Wall-Z samples have been collected.

Espressif's current TFLite Micro support shows that person detection is feasible on ESP32-class devices, but it is substantially heavier than this low-latency motion/grid path. A useful architecture is therefore fast perception continuously plus slower semantic inference when required.

## Build and test

```bat
run_brain_v0.3_tests.cmd
run_brain_v0.3_build.cmd
run_all.cmd
```

Build targets:

```text
src             RA4M1
esp32_r4        onboard ESP32-S3 Brain
esp32_fisheye   fisheye ESP32-CAM
esp32_ps4       existing PS4 ESP32-CAM
```

## Collecting real TinyML training data

Each successful **Teach current view** also appends the current 20x15 grayscale grid to SPIFFS as a labelled sample:

```text
label,<600 hex characters>
```

The web dashboard exposes:

```text
GET  /api/vision/dataset        download wallz_vision_dataset.csv
POST /api/vision/dataset/reset  clear the collected dataset
```

This means the same examples used for immediate on-device prototype learning become a reproducible training corpus for the later CNN. Collect many examples per label under different positions, distances and lighting conditions. Include explicit negative/background classes such as `empty`, `floor` or `other` rather than assuming every frame contains a target.

Example desktop training command after downloading the CSV:

```bat
python -m pip install tensorflow numpy
python tools\train_wallz_visual_tinyml.py wallz_vision_dataset.csv --out wallz_visual_int8.tflite --epochs 30
```

The trainer validates the dataset before importing TensorFlow, trains only on the supplied Wall-Z samples and exports a fully-int8 `.tflite` model plus a `.labels.txt` class list. Brain v0.3 does **not** automatically execute that exported CNN yet; v0.3 establishes the real-data collection and teach-able memory layer needed to do that cleanly in the next inference backend.
