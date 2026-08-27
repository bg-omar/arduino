---
name: Brain v0.7 episodic memory
overview: "BRAIN_v0.6.0_to_v0.7.0 overlay: fisheye PSRAM pre/post episode capture, C,EP / VE,* protocol, S3 episode index + dashboard; v0.6 shared PS4 control and RA safety preserved."
todos:
  - id: apply-verify-commit
    content: Apply v0.7 patch, verify native+builds+safety, write plan, commit Brain v0.7
    status: completed
isProject: true
---

# Brain v0.7.0 episodic memory overlay

Treat `BRAIN_v0.6.0_to_v0.7.0` as an additive overlay on Brain v0.6.

## Overlay

- Fisheye PSRAM rolling pre-buffer (default 2 s pre + trigger + 3 s post @ 5 fps)
- Camera-local episode folders (PGM + `frames.csv`)
- Protocol: `C,EP` request; `VE,BEGIN` / `VE,DONE` feedback (`include/wallz_episode_protocol.h`)
- S3 SPIFFS `/episode_index.csv` + dashboard capture/download/reset
- Auto-triggers: teach/reward, autonomy mode changes, hard stop, obstacle, novel/motion
- `test_brain_v07`, `run_brain_v0.7_*.cmd`, docs

## Kept safety / prior Brain

- `B,BRAKE` + `ra_link::brake()`, `RADAR_MAX_CM = 500`, `brainForwardIsSafe`, poll/tick, `Car_creepBack`
- v0.6 shared PS4 temporary override + OPTIONS autonomy menu
- RA4M1 SPI SD + `SETUP.TXT` unchanged; no raw episode mirroring to RA
- Fisheye SD_MMC 1-bit + GPIO13/4 UART
- `include/secrets.h` local only (not committed)

## Verify (execution)

- native: `test_brain_v07` PASSED; full suite green when re-run (occasional Windows ERRORED on unrelated suites is flake)
- builds: `src` / `esp32_r4` / `esp32_fisheye` SUCCESS
