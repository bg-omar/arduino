@echo off
setlocal
cd /d "%~dp0"
call run_brain_v0.1_tests.cmd || exit /b 1
call run_brain_v0.1_build.cmd || exit /b 1
echo ============================================================
echo Optional nodes: PS4 ESP32-CAM + front ESP32-CAM
echo ============================================================
pio run -e esp32_ps4 || exit /b 1
pio run -e esp32_cam || exit /b 1
echo ============================================================
echo ALL WALL-Z v0.1 BUILDS OK
