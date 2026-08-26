# Brain v0.4.0 PS4 imitation overlay

Treat `BRAIN_v0.3.0_to_v0.4.0.patch` as an additive overlay on Brain v0.3. Observation-only `D,...` side channel + S3 `ImitationMemory`; RA safety core unchanged.

## Overlay (applied)

- RA → S3 `D,ms,lx,ly,rx,ry,drive_active,head_active` (mirror of applied PS4 intent)
- `PS4::manualSnapshot()` + drive-intent tracking
- `src/manual_demo.h`; S3 parse in `ra_link`
- S3: `ImitationMemory` / `imitation_store`, web shadow/policy, SPIFFS `/imitation_dataset.csv`
- Default shadow-only; execute needs Brain ARM + explicit policy + confidence
- Obstacle path brakes first via `ra_link::brake()`
- Docs/scripts/`test_brain_v04`/`train_wallz_imitation_tinyml.py`

## Not overwritten

- `include/secrets.h`
- `src/radar_scan.h`, `src/motor.*`, `src/main_ra.cpp`

## Kept safety fixes

| Fix | Still present |
|-----|----------------|
| Radar max 500 cm | `RADAR_MAX_CM = 500` |
| No-echo sonar = clear | `brainForwardIsSafe` |
| Obstacle Stop does not disarm | `B,BRAKE` + `ra_link::brake()` |
| One brain poll/tick split | `brain_link::poll` early |
| Reverse creep | `Motor::Car_creepBack()` |
| Visual memory (v0.3) | `VisualMemory` intact |

## Verify

- native: 144/144 (incl. `test_brain_v04`)
- `pio run -e src`, `esp32_r4`, `esp32_fisheye`: SUCCESS
