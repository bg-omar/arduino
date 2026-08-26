# Wall-Z Brain v0.4.0 — changelog

## Added

- PS4 imitation-learning side channel: `D,ms,lx,ly,rx,ry,drive_active,head_active` from RA4M1 to onboard ESP32-S3.
- `PS4::manualSnapshot()`; it only exposes the already-applied controller intent and does not alter motor/servo control.
- Bounded `ImitationMemory` on the S3:
  - 96 persistent demonstration samples;
  - 8 quantized world-state features plus current recognized visual-label hash;
  - discrete actions: forward/back, left/right turn, look left/right/up/down;
  - weighted k-nearest-neighbour prediction;
  - duplicate suppression;
  - confidence gate (`680/1000`).
- Persistent NVS store `wallzimit/state04`.
- Automatic labelled PS4 demonstration dataset in SPIFFS: `/imitation_dataset.csv`.
- Web endpoints/dashboard for imitation status, shadow/policy mode, reset and dataset download.
- Optional `tools/train_wallz_imitation_tinyml.py` to compress collected demonstrations into an INT8 TFLite policy later.
- Native v0.4 tests and one-click `run_brain_v0.4_*.cmd` scripts.

## Safety invariants

- Imitation defaults to **shadow-only**.
- Imitation execution requires normal Brain ARM **and** explicit imitation-policy enable.
- PS4/manual input still auto-disarms autonomous execution.
- Existing robot modes still block Brain movement.
- Obstacle context issues `B,BRAKE` before imitation is considered.
- Forward imitation still goes through the RA4M1 `brainForwardIsSafe()` sonar gate.
- Every movement remains a short RA4M1-deadlined pulse.
- Fisheye remains perception-only.

## Preserved v0.2/v0.3 behavior

- `B,BRAKE` does not disarm autonomy.
- radar maximum remains 500 cm.
- no sonar echo within 5 m remains "clear" for the forward gate.
- `brain_link::poll()` / `tick()` sequencing is unchanged.
- `Motor::Car_creepBack()` remains in use for reverse Brain commands.
- visual concept memory and TinyML vision dataset remain intact.
