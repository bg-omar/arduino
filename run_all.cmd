@echo off
setlocal
cd /d "%~dp0"
call run_brain_v0.1_tests.cmd || exit /b 1
call run_brain_v0.2_tests.cmd || exit /b 1
call run_brain_v0.3_tests.cmd || exit /b 1
call run_brain_v0.4_tests.cmd || exit /b 1
call run_brain_v0.5_tests.cmd || exit /b 1
call run_brain_v0.6_tests.cmd || exit /b 1
call run_brain_v0.7_tests.cmd || exit /b 1
call run_brain_v0.7_build.cmd || exit /b 1
echo ============================================================
echo ALL WALL-Z BRAIN v0.7 CORE BUILDS OK
echo ============================================================
