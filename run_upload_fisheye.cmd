@echo off
setlocal
cd /d "%~dp0"
echo ============================================================
echo Wall-Z Fisheye Vision v0.1.0 - upload AI-Thinker ESP32-CAM
echo ============================================================
echo Disconnect the Wall-Z UART wires while flashing if your USB-TTL
 echo setup conflicts with them. Put GPIO0 to GND for download mode,
 echo reset/power-cycle, upload, then remove GPIO0 from GND and reboot.
echo.
pio run -e esp32_fisheye -t upload
