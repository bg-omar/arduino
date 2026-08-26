@echo off
setlocal
cd /d "%~dp0"
echo Uploading RA4M1 robot firmware...
pio run -e src -t upload
