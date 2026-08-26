# Wall-Z v0.5 storage architecture

## Decision

Wall-Z v0.5 uses **local-first camera storage**.

```text
ESP32-CAM fisheye
├─ camera acquisition
├─ motion / grid extraction
├─ microSD: raw visual experience
└─ UART compact telemetry
        ↕
onboard ESP32-S3 Brain
├─ semantic visual memory
├─ novelty / reward
├─ imitation learning
├─ NVS persistent policies
└─ SPIFFS compact training CSVs
        ↕
RA4M1
├─ motors / servos / safety
└─ SPI SD: SETUP.TXT + optional robot/system logs
```

Raw camera frames are **not** routed through the onboard Brain and are **not** forwarded to the RA4M1 SD card.

## Why local-first

A 160x120 grayscale frame is 19,200 bytes before the small PGM header. The UART Brain link is for compact telemetry and semantic context, not bulk image transport. Keeping raw images on the camera avoids:

- UART bandwidth use;
- extra copies through S3 RAM;
- load on the RA4M1 real-time loop;
- coupling camera storage failure to motor/safety control.

## Fisheye SD content

Each boot creates a session directory:

```text
/wallz/session.txt
/wallz/s0001/events.csv
/wallz/s0001/f000001_0000123456.pgm
/wallz/s0001/f000002_0000129021.pgm
...
```

`events.csv` contains:

```text
ms,frame,reason,label,motion,x,y,brightness,contrast,familiarity,novelty,value,brain_fresh,file
```

Raw frames are binary PGM (`P5`) grayscale images. They are directly usable from Python/OpenCV/Pillow and contain the same grayscale signal used by the embedded vision path.

## Storage triggers

The camera can store without Brain:

- significant motion while Brain context is offline;
- bounded by a default 5 s automatic-save cooldown.

When Brain is online it can additionally trigger storage for:

- unknown visual motion;
- visually novel motion;
- `Teach current view`;
- positive/negative reward;
- manual `Store raw frame` from the web dashboard.

The Brain sends only compact messages such as:

```text
C,CTX,person,912,88,250
C,SAVE,teach,person
C,SAVE,reward_pos,person
```

## RA4M1 SD

The existing RA4M1 SPI SD remains unchanged. It retains `SETUP.TXT` / menu configuration and can continue to hold compact robot logs. v0.5 deliberately does not use it as a camera dataset sink.

## Failure isolation

- Fisheye SD missing/failing: vision telemetry continues.
- Brain offline: fisheye can still save high-motion events locally.
- Fisheye offline: RA4M1 motor/safety remains independent.
- RA4M1 SD missing: Brain/fisheye memory and camera SD remain independent.
