# Brain v0.3.0 visual memory overlay

Treat `BRAIN_v0.3.0.patch` as an additive overlay on the merged v0.2 tree. The patch was built from `Wall-Z_source_v0.2-merged` and does not touch RA motor/radar/brain_link safety files.

## Overlay (applied)

- Fisheye: `C,SNAPSHOT` / `C,GRID,<1..5>` + 20x15 grayscale grid snapshots (~2 Hz)
- S3: `VisualMemory` (up to 8 concepts, NVS), familiarity discounts novelty
- Web teach/list/reset + SPIFFS labelled dataset (`/api/vision/dataset`)
- New: `vision_grid_protocol.h`, `visual_memory.h`, `visual_store.*`
- `test/test_brain_v03/`, `tools/train_wallz_visual_tinyml.py`
- Docs/scripts: `CHANGELOG`/`README_BRAIN_v0.3.0`, `run_brain_v0.3_*.cmd`, `run_all.cmd` → v0.3
- `BrainTelemetry`: `vision_familiarity`, `vision_value` (S3-fused only)

## Not overwritten

- `include/secrets.h` (local WiFi kept)
- `src/radar_scan.h`, `src/brain_link.*`, `src/motor.*`, `src/main_ra.cpp`
- Nested empty patch / `__pycache__` from the ChatGPT patch skipped

## Kept safety fixes

| Fix | Still present |
|-----|----------------|
| Radar max 500 cm | `RADAR_MAX_CM = 500` |
| No-echo sonar = clear | `brainForwardIsSafe` |
| Obstacle Stop does not disarm | `B,BRAKE` + `ra_link::brake()` |
| One brain poll/tick split | `brain_link::poll` early |
| Reverse creep | `Motor::Car_creepBack()` |

Fisheye remains perception-only. TinyML trainer is offline; v0.3 does not run a TFLite CNN on-device yet.

## Verify

- native: 140/140 (incl. `test_brain_v03`)
- `pio run -e src`, `esp32_r4`, `esp32_fisheye`: SUCCESS
