@echo off
setlocal
cd /d "%~dp0"
echo ============================================================
echo Wall-Z Brain v0.1.0 - ESP32-S3 upload
echo ============================================================
echo Put the onboard ESP32-S3 in download mode as required by your
 echo current UNO R4 WiFi / ESP_UNO_R4 workflow before continuing.
echo This overwrites the onboard S3 firmware; the project keeps the
 echo CDC/CMSIS-DAP bridge through ESP_UNO_R4.
pause
pio run -e esp32_r4 -t upload
