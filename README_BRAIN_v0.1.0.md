# Wall-Z Brain v0.1.0

This release adds a deliberately conservative cognitive layer to the existing Wall-Z project. Existing motor, servo, PS4, radar, avoidance, display and sensor code remains the robot-control authority on the RA4M1. The onboard ESP32-S3 becomes the higher-level learning / memory processor.

## Architecture

```text
PS4 controller
    |
    v Bluetooth Classic
ESP32-CAM PS4 host                 front ESP32-CAM
(existing firmware unchanged)      (existing camera firmware unchanged)
    | TX -> RX                            |
    v                                     | future vision events
RA4M1 / UNO R4                            |
├─ motors                                 |
├─ PWM / head servos                      |
├─ sonar                                  |
├─ gyro                                   |
├─ light + microphones                    |
├─ existing robot modes                   |
├─ PS4 manual override                    |
└─ Brain safety gate                      |
    | internal UART 115200                |
    v                                     |
ESP32-S3 onboard <-------------------------+
├─ WiFi + NTP + web UI
├─ world-state telemetry
├─ novelty / curiosity / arousal
├─ small online Q-learning table
├─ reward input
├─ persistent NVS memory
└─ optional low-authority autonomy
```

## What v0.1 learns

The brain does **not** claim consciousness and it does not use TensorFlow yet. It creates a small adaptive state from live sensor observations and learns action values using Q-learning:

```text
Q(s,a) <- Q(s,a) + alpha * [r + gamma * max_a Q(s',a) - Q(s,a)]
```

Current discrete contexts:

- calm
- obstacle
- light-left
- light-right
- sound-event
- motion-event

Current possible brain actions:

- idle
- stop
- look-left
- look-right
- creep-forward
- turn-left
- turn-right

The web UI exposes positive and negative reward buttons. Q values and cognitive metrics are persisted in ESP32-S3 NVS so Wall-Z can continue from previous experience after reboot.

## Authority and safety

The design is intentionally asymmetric:

```text
PS4 / human control
        > RA4M1 safety + existing robot modes
        > ESP32-S3 Brain
```

RA4M1 independently enforces:

- brain starts disarmed;
- manual PS4 input immediately disarms Brain;
- existing RA robot modes prevent Brain arming;
- Brain heartbeat timeout: 1200 ms;
- one Brain motor pulse is at most 350 ms;
- forward Brain movement requires a fresh sonar reading;
- minimum forward clearance: 350 mm;
- forward/turn/reverse Brain motion uses creep PWM;
- `STOP` is always accepted;
- head angles are clamped to the existing servo range.

The ESP32-S3 therefore cannot simply leave the drive motors running if its loop, WiFi stack or learning logic fails.

## Internal protocol

RA4M1 -> ESP32-S3 at 10 Hz:

```text
T,ms,distance_mm,light_l,light_r,mic_l,mic_r,gx_mrad,gy_mrad,gz_mrad,head_xy,head_z,manual,robot_mode,brain_armed
```

ESP32-S3 -> RA4M1:

```text
B,HB
B,ARM,0
B,ARM,1
B,STOP
B,HEAD,xy,z
B,MOVE,F,duration_ms
B,MOVE,B,duration_ms
B,MOVE,L,duration_ms
B,MOVE,R,duration_ms
```

The onboard S3 side uses the same `SERIAL_AT` configured by `ESP_UNO_R4`; the RA side uses `SERIAL_AT=Serial2` as already configured in `platformio.ini`.

## Web interface

Open the IP printed by the ESP32-S3 serial monitor. v0.1 adds:

- live RA link state and telemetry;
- sonar distance;
- light and microphone pairs;
- gyro values;
- head servo position;
- manual / existing robot-mode state;
- novelty, curiosity, arousal, confidence and valence;
- current context and suggested action;
- ARM / Disarm / STOP;
- + reward / - reward;
- Reset learning;
- existing WiFi / NTP / heap / log information.

HTTP endpoints:

```text
GET  /api/status
GET  /api/log
GET  /api/robot
GET  /api/brain
POST /api/brain/arm?on=1
POST /api/brain/arm?on=0
POST /api/brain/reward?v=1
POST /api/brain/reward?v=-1
POST /api/brain/reset
POST /api/robot/stop
```

## Build

From a PlatformIO terminal:

```bat
run_brain_v0.1_tests.cmd
run_brain_v0.1_build.cmd
```

or:

```bat
run_all.cmd
```

`run_all.cmd` tests the Arduino-free Brain core and builds all four existing environments:

```text
src          RA4M1 robot
esp32_r4     onboard ESP32-S3 Brain
esp32_ps4    existing PS4 ESP32-CAM host
esp32_cam    existing front camera
```

Uploads remain separate on purpose:

```bat
run_upload_ra.cmd
run_upload_brain_s3.cmd
run_upload_ps4_cam.cmd
```

The onboard S3 must be put into its required download mode before flashing it, exactly as in the existing ESP_UNO_R4 workflow.

## PS4 / motors / servo compatibility

v0.1 does not rewrite the existing control implementation. In particular these existing files are retained as the primary control implementation:

```text
src/PS4.cpp
src/PS4.h
src/motor.cpp
src/motor.h
src/pwm_board.cpp
src/pwm_board.h
src/robot_modes.cpp
src_esp32_ps4/main_ps4.cpp
```

Only `main_ra.cpp` gets Brain scheduler calls, and `config.h` gets `USE_BRAIN_LINK=1`.

## Why the old fisheye ESP32-CAM felt slow

The existing front-camera code is retained unchanged in v0.1, but its old streaming configuration explains much of the latency:

- `FRAMESIZE_SVGA` (800 x 600);
- JPEG encoding/transfer over WiFi;
- only one frame buffer (`fb_count = 1`);
- the MJPEG HTTP handler remains inside a continuous stream loop for the client.

That is appropriate for image quality, not low-latency robot vision. A later camera/vision release should benchmark e.g. QVGA/VGA, two PSRAM frame buffers where supported, grab-latest behavior, and local feature extraction rather than sending every full frame to the Brain.

## TensorFlow / TinyML roadmap

Recommended next stages:

```text
v0.1  telemetry + memory + reward + online adaptive policy       <-- this release
v0.2  fast fisheye vision node + motion/color/face events
v0.3  TinyML/TFLite-Micro classifier on compact sensor features
v0.4  imitation learning from PS4 demonstrations
v0.5  episodic memory + learned person/object associations
```

TensorFlow should consume the same world-state interface; it should not obtain direct motor authority.

## References

```latex
\begin{thebibliography}{9}

\bibitem{WatkinsDayan1992}
C.~J.~C.~H. Watkins and P. Dayan,
``Q-learning,''
\textit{Machine Learning}, vol.~8, pp.~279--292, 1992.
\doi{10.1007/BF00992698}

\bibitem{ArduinoUNOR4WiFi2026}
Arduino,
\textit{UNO R4 WiFi Hardware Documentation},
Arduino Documentation, 2026.
\url{https://docs.arduino.cc/hardware/uno-r4-wifi/}

\bibitem{VshymanskyyESPUNOR4}
V. Shymanskyy,
\textit{ESP\_UNO\_R4: ESP32-S3 USB/CMSIS-DAP support for Arduino UNO R4 WiFi},
GitHub repository, accessed 2026.
\url{https://github.com/vshymanskyy/ESP_UNO_R4}

\end{thebibliography}
```
