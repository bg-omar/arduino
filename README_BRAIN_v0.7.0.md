# Wall-Z Brain v0.7.0 — Episodic Memory + Pre/Post Event Recording

v0.7.0 extends v0.6 shared-control without changing RA4M1 motor/safety authority, PS4 priority, runtime autonomy menu, fisheye wiring, or the RA4M1 SPI SD / `SETUP.TXT` path.

## Architecture

- **RA4M1**: motors, servos, sonar/sensors, hard safety, original robot modes, PS4 actuator priority, `SETUP.TXT` on existing SPI SD.
- **Onboard ESP32-S3 Brain**: world-state/learning, visual concepts, imitation policy, WiFi dashboard, compact episode index in SPIFFS.
- **Fisheye ESP32-CAM**: 160×120 grayscale perception, local microSD, rolling visual pre-buffer, full episode frames.
- **PS4 ESP32-CAM**: unchanged controller receiver / UART bridge.

## Episode window

Default episode recording:

- rolling pre-buffer: **2.0 s**
- sample rate: **5 fps**
- trigger frame: **1 frame**
- post-event: **3.0 s @ 5 fps**

At 160×120 grayscale, the default 2 s pre-history is 10 frames ≈ 192 kB. The implementation allocates a 25-frame PSRAM ring (up to 5 s requested pre-history) when PSRAM is available.

## Episode triggers

Brain/S3-triggered:

- `teach`
- `reward_pos`, `reward_neg`
- `brain_arm`, `brain_disarm`
- `mode_brain`, `mode_imitation`, `mode_sense`, `mode_roam`, `mode_stop`
- `hard_stop`
- `obstacle` (latched: one episode per obstacle encounter)
- manual dashboard episode capture

Camera-local triggers remain available when Brain is offline or reports high novelty:

- `motion_offline`
- `unknown`
- `novel`

## Camera-SD layout

```text
/wallz/
  session.txt
  s0001/
    events.csv
    ep000001/
      frames.csv
      pre_000_0000123000.pgm
      ...
      event_0000125000.pgm
      post_000_0000125200.pgm
      ...
```

`frames.csv` records phase (`pre`, `event`, `post`), timestamp relative to the trigger, motion/attention metrics and file path. Raw frames never cross the S3/RA UART links.

## S3 compact episode index

The onboard ESP32-S3 writes `/episode_index.csv` to SPIFFS. Request rows contain the multimodal trigger state:

- sonar distance
- light L/R
- microphone L/R
- gyro XYZ
- head XY/Z
- visual motion and attention
- visual familiarity / label

Camera `BEGIN`/`DONE` rows add camera session/episode identifiers, frame count and error count.

Dashboard endpoints:

- `POST /api/vision/episode` — capture default 2 s + event + 3 s episode
- `GET /api/episodes/index` — download `wallz_episode_index.csv`
- `POST /api/episodes/index/reset` — clear S3 episode index only

## UART protocol additions

S3 → fisheye:

```text
C,EP,<reason>,<label>,<pre_ms>,<post_ms>
```

Fisheye → S3:

```text
VE,BEGIN,<session>,<episode>,<reason>,<label>,<pre_frames>,<post_target>
VE,DONE,<session>,<episode>,<reason>,<label>,<total_frames>,<errors>
VE,BUSY,<episode>
VE,ERROR
```

## Safety invariants retained from v0.6

- PS4 may stay connected permanently.
- Active PS4 input temporarily owns actuators; Brain remains armed and resumes when input is neutral.
- `OPTIONS` remains explicit autonomy stop + runtime autonomy menu.
- RA4M1 remains final motor/safety authority.
- `B,BRAKE`, 350 mm forward clearance, 500 cm radar maximum, heartbeat/deadlines and `Car_creepBack()` are unchanged.
- Fisheye remains perception/storage-only and has no motor authority.
- RA4M1 SPI SD and `SETUP.TXT` are unchanged.

## Build / test

```bat
run_brain_v0.7_tests.cmd
run_brain_v0.7_build.cmd
run_all.cmd
```

`run_all.cmd` runs v0.1–v0.7 native tests and then builds RA4M1, onboard S3, fisheye and PS4 ESP32-CAM targets.
