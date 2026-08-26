# Changelog — Wall-Z Brain v0.1.0

## Added

- RA4M1 <-> onboard ESP32-S3 Brain telemetry protocol.
- 10 Hz structured sensor/world-state telemetry.
- 4 Hz Brain heartbeat.
- RA-side Brain command parser and independent safety gates.
- Manual PS4 override auto-disarm.
- Existing autonomous robot-mode interlock.
- 350 ms maximum Brain motor pulse.
- 350 mm + fresh-sonar forward-motion gate.
- ESP32-S3 `BrainCore`: novelty, curiosity, arousal, confidence, valence.
- Small online Q-learning policy.
- Positive/negative web reward input.
- NVS persistence of learned Q-table/state.
- Brain/robot JSON APIs and expanded web dashboard.
- Native tests for protocol, safety context classification, reward learning and persistence representation.
- Windows build/test/upload helpers.

## Preserved

- Existing PS4 ESP32-CAM protocol and controller firmware.
- Existing RA4M1 PS4 parsing and tank-drive behavior.
- Existing motor implementation.
- Existing head-servo implementation.
- Existing radar, avoid, follow-light, dancing and sense-react behavior.
- Existing front ESP32-CAM firmware.
- Existing ESP32-S3 WiFi/NTP/HTTP gateway functionality.
- ESP_UNO_R4 CDC/CMSIS-DAP bridge setup.

## Deliberately deferred

- TensorFlow Lite Micro.
- Camera neural inference.
- Person/face identity memory.
- MQTT Brain topics.
- Speech recognition.
- LLM/cloud cognition.

These are deferred until the control/telemetry/learning substrate is hardware-validated.
