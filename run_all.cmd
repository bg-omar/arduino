@echo off
setlocal
cd /d "%~dp0"
call run_brain_v0.1_tests.cmd || exit /b 1
call run_brain_v0.2_tests.cmd || exit /b 1
call run_brain_v0.3_tests.cmd || exit /b 1
call run_brain_v0.4_tests.cmd || exit /b 1
call run_brain_v0.4_build.cmd || exit /b 1
echo ============================================================
echo Existing PS4 ESP32-CAM host
echo ============================================================
pio run -e esp32_ps4 || exit /b 1
echo ============================================================
echo ALL WALL-Z BRAIN v0.4 CORE BUILDS OK
