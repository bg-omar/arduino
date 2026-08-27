# CHANGELOG — Wall-Z Brain v0.7.0

## Added

- fisheye PSRAM rolling pre-event buffer;
- default 2 s pre + trigger + 3 s post episode recording at 5 fps;
- camera-local episode directories with PGM frames and `frames.csv`;
- `C,EP` request protocol and `VE,BEGIN` / `VE,DONE` feedback;
- S3 `/episode_index.csv` with multimodal trigger state;
- automatic episode triggers for teach/reward, autonomy mode changes, hard stop and obstacle encounters;
- camera-local unknown/novel/motion triggers now create episodes instead of isolated auto-save frames;
- dashboard manual episode capture and episode-index download/reset;
- v0.7 native protocol/timing tests;
- v0.7 build/test scripts and `run_all.cmd` integration.

## Preserved

- v0.6 shared PS4/Brain control arbitration;
- PS4 connected permanently; only active input overrides Brain;
- OPTIONS autonomy stop/menu behavior;
- RA4M1 safety and motor authority;
- original Sense React / Free Roam modes;
- fisheye SD_MMC 1-bit wiring and GPIO13/4 UART;
- RA4M1 SPI SD + `SETUP.TXT`;
- visual memory, Q-learning and imitation learning.
