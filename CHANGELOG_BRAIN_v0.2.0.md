# Wall-Z Brain v0.2.0 changelog

## Added

- Dedicated `esp32_fisheye` PlatformIO environment.
- AI-Thinker OV2640 low-latency grayscale capture at QQVGA 160x120.
- PSRAM double buffering and latest-frame capture.
- 20x15 local visual grid and frame-difference motion detector.
- Visual attention centroid x/y, brightness, contrast and measured FPS.
- Full-duplex fisheye UART protocol at 230400 baud.
- Onboard ESP32-S3 `vision_link` on exposed GPIO41/GPIO42.
- `/api/vision` dashboard endpoint and live camera status card.
- Runtime fisheye ping and threshold control.
- Visual novelty/arousal fusion into BrainCore.
- Visual motion attention suggestions (`look-left` / `look-right`).
- Host tests for vision protocol and visual attention behavior.

## Preserved

- Existing PS4 controls.
- Existing motor functions and PWM behavior.
- Existing head/servo implementation.
- Existing RA4M1 sonar and safety ownership.
- Existing robot modes.
- Existing PS4 ESP32-CAM firmware.
- Existing legacy ESP32-CAM source retained as `esp32_cam`.
- Brain v0.1 Q-table persistence format remains compatible because the context/action dimensions were not changed.

## Security / packaging

- Real WiFi credentials are not included in this release. `include/secrets.h` contains placeholders.

## Safety

- Fisheye data is perception-only.
- Fisheye has no direct motor connection or motor authority.
- Brain remains lower priority than PS4/manual, RA safety and existing robot modes.
