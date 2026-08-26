@echo off
setlocal
cd /d "%~dp0"
where pio >nul 2>nul
if errorlevel 1 (
  echo [ERROR] PlatformIO CLI ^(pio^) not found in PATH.
  echo Open a PlatformIO terminal or install PlatformIO Core first.
  exit /b 1
)
echo ============================================================
echo Wall-Z Brain v0.4.0 - PS4 imitation learning tests
echo ============================================================
pio test -e native -f test_brain_v04
exit /b %errorlevel%
