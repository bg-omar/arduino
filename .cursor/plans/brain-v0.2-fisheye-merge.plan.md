# Brain v0.2.0 as overlay (keep v0.1 fixes)

Treat `Wall-Z_Brain_v0.2.0.zip` as an additive patch, not a full-tree replace. The zip is a ChatGPT snapshot based on v0.1 **without** the later RA safety fixes.

## Overlay (copied)

- `src_esp32_fisheye/` — QQVGA grayscale, UART2 230400, GPIO13/14
- `src_esp32_r4/vision_link.*`, `vision_protocol.h`
- `src_esp32_r4/brain_core.h` — vision novelty/arousal + look-left/right on motion
- `src_esp32_r4/main_esp.cpp` from zip, then restore `ra_link::brake()` on `BrainAction::Stop`
- `test/test_brain_v02/`, `tools/brain_test_standalone.cpp`
- v0.2 docs/scripts + `[env:esp32_fisheye]` in `platformio.ini`
- `BrainTelemetry` vision fields (`vision_online/motion/x/y/brightness/contrast`)

## Not overwritten

- `include/secrets.h`
- `src/radar_scan.h` (500 cm)
- `src/brain_link.cpp/.h`, `src/motor.cpp/.h`, `src/main_ra.cpp` (`poll`/`tick`, `B,BRAKE`, `Car_creepBack`)
- `.cursor/plans`

## Kept v0.1 fixes

| Fix | Still present |
|-----|----------------|
| Radar max 500 cm | `RADAR_MAX_CM = 500` |
| No-echo sonar = clear | `brainForwardIsSafe(distance_mm < 0)` |
| Obstacle Stop does not disarm | `B,BRAKE` + `ra_link::brake()` |
| One brain poll/tick split | `brain_link::poll` early, `tick` later |
| Reverse creep | `Motor::Car_creepBack()` |

Fisheye is perception-only: no motor authority. RA `SERIAL_AT=Serial2` stays RA↔S3; S3 Serial1 = RA, Serial2 = fisheye.

## Verify

- native: 137/137
- `pio run -e src`, `esp32_r4`, `esp32_fisheye`: SUCCESS
