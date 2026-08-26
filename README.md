This is my source code for my Arduino uno R4 Wifi robot

|Env| Folder|
|------|-----|
|src|./src|
|esp32_cam|./src_esp32_cam|
|esp32_ps4|./src_esp32_ps4|
|esp32_r4|./src_esp32_r4|


The Arduino's onboard ESP32-S3 is programmed seperatly [src_esp32_r4],
We soldered a ESP32-cam to the MotorShield to receive PS4-controller commands [src_esp32_ps4]
We have an other ESP32-Cam on the front to have a video feed [src_esp32_cam]
 

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

Current cognitive/vision extension: `README_BRAIN_v0.3.0.md`.

The dedicated fisheye node is `src_esp32_fisheye` and remains perception-only. It sends fast motion telemetry plus low-rate 20x15 grayscale learning snapshots directly to the onboard ESP32-S3. The S3 can persistently learn up to 8 named visual concepts from the web dashboard.

## Brain v0.4

Current learning extension: `README_BRAIN_v0.4.0.md`.

Brain v0.4 keeps the v0.3 fisheye visual memory and adds bounded, persistent PS4 imitation learning on the onboard ESP32-S3. Manual PS4 control remains direct on the RA4M1; a new observation-only `D,...` UART side channel mirrors stick intent to the S3 so it can learn state -> action examples. Imitation starts in shadow mode and can execute only when Brain is separately armed, imitation policy is explicitly enabled, confidence passes the gate, and all RA4M1 safety checks allow the command.
