# Wall-Z Brain v0.2.0 + Fisheye Vision v0.1.0

This release extends Wall-Z Brain v0.1.0 with a dedicated low-latency ESP32-CAM fisheye vision node while preserving the existing RA4M1 motor/servo/safety layer and the separate PS4 ESP32-CAM node.

## Architecture

```text
PS4 controller
    |
    | Bluetooth Classic
    v
ESP32-CAM (existing PS4 node)
    |
    | existing UART
    v
RA4M1 / UNO R4
    |- motor PWM
    |- servos / head
    |- sonar
    |- sensors
    |- manual override
    `- hard safety
         ^  |
         |  | internal UART
         |  v
      onboard ESP32-S3 Brain
         |- WiFi/web dashboard
         |- memory / Q-learning
         |- visual fusion
         `- low-authority suggestions
              ^  |
              |  | UART2 @ 230400, 3.3 V
              |  v
       ESP32-CAM Fisheye Vision
         |- OV2640 fisheye
         |- 160x120 grayscale
         |- 20x15 local visual grid
         |- frame-difference motion
         |- visual attention centroid
         `- brightness / contrast / FPS
```

## Why UART is used

UART carries only compact visual telemetry, never image frames. This avoids the latency and bandwidth cost of the previous continuous MJPEG stream.

Basic one-way operation only requires fisheye TX -> Brain S3 RX + common GND. This release uses full duplex because the Brain can also change the camera threshold/rate and ping the node at runtime.

## Wiring: fisheye ESP32-CAM directly to UNO R4 onboard ESP32-S3

Both sides are 3.3 V logic, so no level shifter is required for these two ESP devices.

```text
AI-Thinker ESP32-CAM                 UNO R4 WiFi ESP header
--------------------                 ----------------------
GPIO14 / UART2 TX   ---------------> pin 2 / ESP32-S3 GPIO41 RX
GPIO13 / UART2 RX   <--------------- pin 1 / ESP32-S3 GPIO42 TX
GND                 ---------------- pin 6 / GND
```

Do not connect these UART signals to 5 V. Power the ESP32-CAM from a stable 5 V rail; do not power the camera module from a weak 3.3 V GPIO/regulator path. Grounds of the robot, UNO and fisheye module must be common.

GPIO13/GPIO14 are used because this dedicated camera node does not use the ESP32-CAM SD-card interface.

## Low-latency camera mode

The old camera code used SVGA and one framebuffer while serving continuous MJPEG. The new dedicated node instead uses:

- `PIXFORMAT_GRAYSCALE`
- `FRAMESIZE_QQVGA` = 160 x 120
- 2 framebuffers when PSRAM is present
- `CAMERA_GRAB_LATEST`
- target rate 20 FPS
- 20 x 15 grid (300 cells)
- four pixel samples per grid cell
- frame-difference motion score 0..1000
- motion centroid x/y -1000..1000
- brightness and sampled contrast

This is intentionally a local perception node rather than a network camera.

## Vision protocol

Fisheye -> Brain S3:

```text
V,ms,motion,x,y,brightness,contrast,fps_x10,flags
```

Example:

```text
V,48320,92,-440,80,126,51,203,7
```

Meaning: motion=92/1000, attention left of center, 20.3 FPS.

Brain S3 -> Fisheye:

```text
C,PING
C,THR,18
C,RATE,20
C,DEBUG,1
```

## Cognitive fusion

Fisheye telemetry is fused only on the ESP32-S3. The RA4M1 protocol and hard motor safety remain independent.

Visual motion contributes to Brain novelty and arousal. A strong motion event can suggest `look-left` or `look-right` from its centroid. It does not gain direct motor authority.

Authority remains:

```text
PS4/manual > RA4M1 safety > existing robot modes > ESP32-S3 Brain > fisheye perception
```

## Existing controls preserved

The following files are unchanged from Brain v0.1.0:

```text
src/PS4.cpp
src/motor.cpp
src/pwm_board.cpp
src/robot_modes.cpp
src_esp32_ps4/main_ps4.cpp
src_esp32_cam/main.cpp
src_esp32_cam/handlers.cpp
src_esp32_cam/WifiCam.hpp
```

The existing PS4 camera node and the old network-camera source therefore remain available as separate PlatformIO environments.

## PlatformIO environments

```text
src             RA4M1 robot controller
esp32_r4        onboard ESP32-S3 Brain
esp32_ps4       existing PS4 ESP32-CAM node
esp32_cam       existing legacy camera source
esp32_fisheye   NEW low-latency fisheye ESP32-CAM
native          host tests
```

## Build

```bat
run_brain_v0.2_tests.cmd
run_brain_v0.2_build.cmd
```

or everything:

```bat
run_all.cmd
```

Upload fisheye:

```bat
run_upload_fisheye.cmd
```

Upload onboard Brain:

```bat
run_upload_brain_s3.cmd
```

## WiFi secrets

This package intentionally contains no real WiFi password. Edit `include/secrets.h` locally before building the onboard ESP32-S3 / legacy WiFi camera.

## Dashboard additions

`/api/vision` reports:

```text
online
age_ms
motion
x
y
brightness
contrast
fps_x10
flags
message
```

The web dashboard displays the same data live and exposes camera ping/default-threshold controls.

## First hardware test

1. Flash `esp32_fisheye` with UART wires disconnected if required by your USB-TTL programmer.
2. Remove GPIO0 from GND and reboot the ESP32-CAM normally.
3. Connect GND, TX14 -> S3 GPIO41, RX13 <- S3 GPIO42.
4. Flash/run `esp32_r4` Brain v0.2.0.
5. Open the Brain web dashboard.
6. Move a hand through the fisheye image. `motion` should increase and x should change sign left/right.
7. Keep Brain autonomy DISARMED until the vision telemetry behaves correctly.

