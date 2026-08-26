# Wall-Z Brain v0.5.0 — local visual experience on fisheye SD

v0.5 separates **raw visual experience** from **semantic Brain memory**.

## Architecture

```text
fisheye ESP32-CAM
  160x120 grayscale
  motion / 20x15 grid
  local microSD raw PGM frames
          |
          | V/G compact telemetry
          v
onboard ESP32-S3 Brain
  visual recognition
  novelty / reward
  imitation learning
  NVS + SPIFFS
          |
          v
RA4M1
  hard real-time motor/servo/safety
  existing SPI SD: SETUP.TXT + compact logs
```

The raw image path ends at the fisheye microSD. It does not pass through the Brain or RA4M1.

## Local autonomous recording

The camera does **not** need permission from Brain for every frame. It can independently save significant motion when Brain context is unavailable.

When Brain is online, the S3 returns semantic context:

```text
C,CTX,<label>,<familiarity>,<novelty>,<value>
```

Examples:

```text
C,CTX,unknown,0,1000,0
C,CTX,person,912,88,250
```

This lets the camera preferentially retain unknown/novel events without moving raw image data over UART.

## Explicit semantic captures

Brain may request the next current raw frame be written:

```text
C,SAVE,teach,person
C,SAVE,reward_pos,person
C,SAVE,reward_neg,unknown
C,SAVE,manual,ball
```

`Teach current view` therefore now creates both:

1. a compact labelled 20x15 training sample in S3 SPIFFS; and
2. a raw 160x120 PGM frame on the fisheye SD.

## Camera SD layout

```text
/wallz/session.txt
/wallz/s0001/events.csv
/wallz/s0001/f000001_0000123456.pgm
...
```

The `events.csv` ledger keeps reason, Brain label, familiarity, novelty, reward value and motion metadata next to each raw frame.

## Default automatic policy

- Brain offline + motion >= 220/1000 -> save, max once per 5 s.
- Brain online + motion >= 90 and familiarity < 760/1000 -> save.
- Brain online + motion >= 90 and novelty >= 560/1000 -> save.
- Explicit teach/reward/manual saves have a short 350 ms anti-spam gate.

These values are deliberately conservative defaults, not trained thresholds.

## SD + UART pin change

The AI-Thinker slot is mounted with:

```cpp
SD_MMC.begin("/sdcard", true); // 1-bit mode
```

Therefore:

```text
SD: GPIO14 CLK, GPIO15 CMD, GPIO2 D0
UART: GPIO13 TX -> S3 GPIO41 RX
      GPIO4  RX <- S3 GPIO42 TX
```

See `WIRING_FISHEYE_SD_v0.5.0.txt`.

## Dashboard additions

The onboard S3 dashboard now reports:

- camera SD mounted/missing;
- total/used MB;
- raw frames stored this session;
- event count;
- SD error count.

Controls:

- `Store raw frame`
- `Camera SD auto ON`
- `Camera SD auto OFF`
- existing teach/reward operations automatically request semantically labelled raw captures.

## RA4M1 SD remains unchanged

The existing Arduino SPI SD remains the owner of `SETUP.TXT` and compact robot/config logs. v0.5 does not duplicate camera PGM data there.

## Build/test

```bat
run_brain_v0.5_tests.cmd
run_brain_v0.5_build.cmd
run_all.cmd
```

Actual target compilation still requires PlatformIO on the development machine.
