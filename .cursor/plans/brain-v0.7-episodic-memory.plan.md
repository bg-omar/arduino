---
name: Brain v0.7 episodic memory
overview: "Brain v0.7.0 overlay: fisheye PSRAM pre/post episode capture, C,EP/VE protocol, S3 episode index + dashboard."
todos:
  - id: apply-verify-commit
    content: Apply v0.7 patch, Unity test fix, verify builds/safety, commit
    status: completed
isProject: true
---

# Brain v0.7.0 episodic memory overlay

Treat `BRAIN_v0.6.0_to_v0.7.0` as an additive overlay on Brain v0.6.

## Overlay

- Fisheye PSRAM rolling pre-buffer (~2 s pre + trigger + 3 s post @ 5 fps)
- Camera-local episode dirs with PGM + `frames.csv`
- Protocol: `C,EP` request; `VE,BEGIN` / `VE,DONE` feedback (`wallz_episode_protocol.h`)
- S3 `/episode_index.csv` + dashboard episode capture / download / reset
- Auto-triggers: teach/reward, autonomy changes, hard stop, obstacles, novel/motion
- `test_brain_v07` (Unity), `run_brain_v0.7_*.cmd`

## Kept safety

- `B,BRAKE` / RA brake path, `RADAR_MAX_CM = 500`, `brainForwardIsSafe`, poll/tick, `Car_creepBack`
- v0.6 shared PS4 / OPTIONS autonomy menu
- `include/secrets.h` not committed

## Verify

- native tests green (incl. `test_brain_v07`; Windows file-lock flakes retried OK)
- `src` / `esp32_r4` / `esp32_fisheye` SUCCESS
