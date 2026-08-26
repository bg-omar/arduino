@echo off
setlocal
cd /d "%~dp0"
call run_brain_v0.1_tests.cmd || exit /b 1
call run_brain_v0.2_tests.cmd || exit /b 1
call run_brain_v0.3_tests.cmd || exit /b 1
call run_brain_v0.4_tests.cmd || exit /b 1
call run_brain_v0.5_tests.cmd || exit /b 1
call run_brain_v0.5_build.cmd || exit /b 1
echo ============================================================
echo ALL WALL-Z BRAIN v0.5 CORE BUILDS OK
echo ============================================================
