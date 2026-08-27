@echo off
setlocal
cd /d "%~dp0"
where pio >nul 2>nul
if errorlevel 1 (
  echo [ERROR] PlatformIO CLI ^(pio^) not found in PATH.
  exit /b 1
)
echo ============================================================
echo Wall-Z Brain v0.7.0 - episodic memory tests
echo ============================================================
pio test -e native -f test_brain_v07
exit /b %errorlevel%
