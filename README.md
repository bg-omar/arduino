This is my source code for my Arduino uno R4 Wifi robot

|Env| Folder|
|------|-----|
|src|./src|
|esp32_cam|./src_esp32_cam|
|esp32_ps4|./src_esp32_ps4|
|esp32_r4|./src_esp32_r4|
|esp32_fisheye|./src_esp32_fisheye|


The Arduino's onboard ESP32-S3 is programmed seperatly [src_esp32_r4],
We soldered a ESP32-cam to the MotorShield to receive PS4-controller commands [src_esp32_ps4]
We have a dedicated ESP32-CAM fisheye vision/storage node [src_esp32_fisheye]. Its onboard microSD stores raw visual experience locally in Brain v0.5.
The older network camera experiment remains in [src_esp32_cam].
 

|Lib-Module| Type|
|------|-----|
|ADS1X15 | Analog|
|BMP280| Barometer|
|HMC5883| compas|
|SH1106| Oled Display|
|MPU6050|Gyroscope|
|Sonar | Distance calculation|
|DotMatrix| for showing emotions|
|PS4| Using ESP32 Cam TX-RX via Serial2 |
|PWMServoDriver x 16| front servos, laser,  rgb-led|
|SdFat| for save and loading setup of module booleans |
|Menu| for setup of SD and logging|
|Mic Module| left and right|
|Light Sensors| Left and right|

## Brain v0.3

Visual concept memory: `README_BRAIN_v0.3.0.md`.

## Brain v0.4

PS4 imitation learning: `README_BRAIN_v0.4.0.md`.

## Brain v0.5

Current storage/vision extension: `README_BRAIN_v0.5.0.md`.

Brain v0.5 keeps the v0.4 safety and imitation architecture, adds local-first raw visual storage to the dedicated fisheye ESP32-CAM microSD, and sends only compact vision/semantic messages between camera and onboard ESP32-S3. The existing RA4M1 SPI SD remains dedicated to `SETUP.TXT`, menu configuration and compact robot logs.
