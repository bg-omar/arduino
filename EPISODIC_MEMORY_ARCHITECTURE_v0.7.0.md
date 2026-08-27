# Wall-Z v0.7 Episodic Memory Architecture

## Data locality

```text
Fisheye ESP32-CAM                     Onboard ESP32-S3
------------------                    -----------------
20 fps perception                     semantic context
5 fps rolling PSRAM history           multimodal trigger index
full PGM episode frames               episode request/done ledger
        |                                      |
        +----- compact UART protocol ----------+

RA4M1
-----
SETUP.TXT + robot configuration on existing SPI SD
NO raw camera episode mirroring
```

## Why local pre-buffering

A raw 160×120 grayscale frame is 19,200 bytes. Sending that over the 230,400 baud Brain UART would take roughly 0.83 s per frame before framing overhead, so raw episodic data stays on the camera.

At 5 fps:

- 2 s pre-event = 10 frames = 192,000 bytes
- trigger frame = 19,200 bytes
- 3 s post-event ≈ 15 frames = 288,000 bytes
- typical episode payload ≈ 499,200 bytes plus PGM headers/CSV metadata

The camera captures frames continuously at its normal perception rate, but only copies 5 fps into the PSRAM ring and writes to SD after a trigger.

## Episode semantics

An episode is not merely a video burst. It has:

1. **before** — visual state leading into the event;
2. **trigger** — reward, obstacle, teach, mode change, stop, novelty, etc.;
3. **after** — visual consequence for the next bounded interval;
4. **S3 context** — compact nonvisual sensor state at trigger time;
5. **outcome hooks** — camera completion/frame/error counts for later consolidation.

This is the storage substrate for v0.8 WorldState/sensor-confidence and later outcome/causal learning.
