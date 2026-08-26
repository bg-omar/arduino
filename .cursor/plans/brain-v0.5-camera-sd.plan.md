# Brain v0.5.0 fisheye local SD overlay

Treat `BRAIN_v0.4.0_to_v0.5.0.patch` as an additive overlay on Brain v0.4.

## Overlay

- Fisheye SD_MMC 1-bit (GPIO14 CLK / GPIO15 CMD / GPIO2 D0)
- Local PGM frames + `events.csv`; no raw frames over UART/RA
- Protocols: `C,CTX`, `C,SAVE`, `VS,SD`
- UART rewire: camera GPIO13 TX / GPIO4 RX ↔ S3 GPIO41/42
- Fixed native `test_brain_v05` to use Unity `main()` + `-I include`

## Kept safety

BRAKE, 500 cm radar, `brainForwardIsSafe`, poll/tick, visual memory, imitation, secrets untouched.

## Verify

- native 149/149; `src` / `esp32_r4` / `esp32_fisheye` SUCCESS
- Hardware: do not keep old GPIO14 UART wiring; avoid flash LED on GPIO4
