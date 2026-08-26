# Wall-Z Brain v0.3.0

- Added 20x15 fisheye grayscale grid snapshots at 1–5 Hz; default 2 Hz.
- Added `C,SNAPSHOT` and `C,GRID,<fps>` commands.
- Added persistent `VisualMemory` with up to 8 teach-able concepts.
- Added brightness normalization and ±1-cell translation-tolerant recognition.
- Added per-concept learned reward/value association.
- Added web UI/API for teaching, listing and clearing visual concepts.
- Brain novelty now discounts familiar visual motion.
- Added TensorFlow/Keras fully-int8 training/export script for future semantic CNN deployment using real Wall-Z data.
- Preserved all merged v0.2 safety, radar, sonar, PS4, motor, servo and `B,BRAKE` behavior.
- Added SPIFFS-backed labelled vision dataset capture on every successful teach operation.
- Added `/api/vision/dataset` download and `/api/vision/dataset/reset` endpoints plus dashboard controls.
- TensorFlow trainer now accepts the downloaded `label,<600hex>` dataset directly and validates it before importing TensorFlow.
