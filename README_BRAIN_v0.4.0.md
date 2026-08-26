# Wall-Z Brain v0.4.0 — PS4 imitation learning

Brain v0.4 learns from **how you actually drive Wall-Z**. The control hierarchy is unchanged:

```text
PS4/manual + RA4M1 safety
        > existing robot modes
        > armed Brain policy
        > fisheye perception
```

## Data path

```text
PS4 -> ESP32-CAM PS4 host -> UART -> RA4M1
                                  |  existing motor/head control (unchanged)
                                  |
                                  +-> D,ms,lx,ly,rx,ry,drive,head
                                              |
                                              v
                                      onboard ESP32-S3
                                      ImitationMemory
                                      + world state
                                      + visual concept
```

The `D,...` packet is a **mirror of manual intent**, not a command channel.

## What is learned

For an accepted demonstration, the S3 stores a compact feature vector derived from:

- sonar distance;
- left/right light balance;
- left/right microphone balance;
- gyro magnitude;
- current head pan;
- fisheye attention X;
- fisheye motion strength;
- current visual familiarity;
- current recognized visual-label hash.

It associates that state with one of:

```text
forward
backward
turn-left
turn-right
look-left
look-right
look-up
look-down
```

Memory is bounded to 96 examples. Near-identical held-stick samples are rejected.

## Shadow mode first

Imitation policy starts disabled. Drive the robot normally. The dashboard shows what Wall-Z predicts it would do.

When enough examples exist, enable **Use imitation**. This still does nothing unless normal Brain autonomy is separately **ARMED**.

An imitation action is eligible only when:

```text
Brain armed
AND imitation policy enabled
AND prediction confidence >= 680/1000
AND no manual control
AND no existing robot mode
AND not obstacle context
```

The RA4M1 then performs its normal final safety gates. Forward motion still requires sonar clearance and all movement is short/deadlined.

## Dashboard / API

- `GET /api/imitation` — sample count, prediction, confidence, demo state.
- `POST /api/imitation/policy?on=1` — allow confident imitation while Brain is armed.
- `POST /api/imitation/policy?on=0` — shadow-only.
- `POST /api/imitation/reset` — clear on-device imitation memory.
- `GET /api/imitation/dataset` — download `wallz_imitation_dataset.csv`.
- `POST /api/imitation/dataset/reset` — clear only the CSV dataset.

## Optional offline TinyML policy

v0.4 does **not** require TensorFlow. The bounded k-NN policy runs directly on the S3.

For a later compact neural policy:

```bat
python -m pip install tensorflow numpy
python tools\train_wallz_imitation_tinyml.py wallz_imitation_dataset.csv --out wallz_imitation_int8.tflite --epochs 40
```

The script exports an all-INT8 TFLite action classifier plus label and normalization files.

## Build/test

```bat
run_brain_v0.4_tests.cmd
run_brain_v0.4_build.cmd
run_all.cmd
```

`run_all.cmd` retains all v0.1-v0.3 native tests and also builds the existing PS4 ESP32-CAM host.
