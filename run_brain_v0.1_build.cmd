@echo off
setlocal
cd /d "%~dp0"
where pio >nul 2>nul
if errorlevel 1 (
  echo [ERROR] PlatformIO CLI ^(pio^) not found in PATH.
  exit /b 1
)
echo ============================================================
echo Wall-Z Brain v0.1.0 - build RA4M1 + onboard ESP32-S3
echo ============================================================
echo [1/2] RA4M1
pio run -e src || exit /b 1
echo [2/2] ESP32-S3 Brain
pio run -e esp32_r4 || exit /b 1
echo BUILD OK
