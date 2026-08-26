@echo off
setlocal
cd /d "%~dp0"
echo Uploading existing ESP32-CAM PS4 host firmware (unchanged)...
pio run -e esp32_ps4 -t upload
