# Wall-Z Brain v0.5.0 changelog

## Added

- Local microSD storage on the dedicated fisheye ESP32-CAM.
- SD_MMC 1-bit mode to coexist with camera + bidirectional Brain UART.
- Raw 160x120 grayscale PGM event frames.
- Per-session `events.csv` semantic metadata ledger.
- Local-first camera storage policy that still works if Brain is offline.
- Brain -> fisheye semantic context protocol (`C,CTX,...`).
- Brain -> fisheye explicit storage protocol (`C,SAVE,...`).
- Fisheye -> Brain SD status protocol (`VS,SD,...`).
- Web dashboard SD status, manual raw capture, auto-storage enable/disable.
- Teach/reward operations now request labelled raw captures on the fisheye SD.
- Native unit/standalone tests for storage protocol and policy.

## Changed

- Fisheye UART pins changed because GPIO14 is required as SD clock:
  - camera GPIO13 TX -> S3 GPIO41 RX
  - camera GPIO4 RX <- S3 GPIO42 TX
- Fisheye version raised to v0.5.0.
- Brain version raised to v0.5.0.

## Explicitly unchanged

- RA4M1 SPI SD ownership of SETUP.TXT/config remains unchanged.
- No raw camera data is forwarded to the RA4M1 SD.
- PS4 ESP32-CAM remains free of SD responsibilities.
- PS4/manual remains highest user-control authority.
- RA4M1 remains final motor/servo/safety authority.
- Existing BRAKE, sonar-forward, radar, heartbeat and movement-deadline safety behavior remains unchanged.
