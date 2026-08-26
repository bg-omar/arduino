---
name: Src env upgrade upload
overview: "`[env:src]` opschonen, SdFat/SdFs-clash fixen, PlatformIO Arduino-core pinnen op renesas-ra@1.9.0 (ArduinoCore 1.6.0). Bootloader op de chip niet flashen. ESP32 firmware 0.6.0 blijft. Daarna RA-sketch uploaden."
todos:
  - id: native-tests-before
    content: pio test -e native als baseline vóór ini-wijziging
    status: completed
  - id: pin-renesas-core
    content: platform = renesas-ra@1.9.0 (ArduinoCore 1.6.0); bootloader op de chip niet flashen
    status: completed
  - id: trim-lib-deps
    content: "[env:src] lib_deps trimmen; SdFs + Arduino SD weg; lib_ignore SdFs/SD; versiepins gelijkzetten"
    status: completed
  - id: clean-stale-libdeps
    content: Stale .pio/libdeps/src (vooral SdFs) weg zodat LDF oude headers niet blijft vinden
    status: completed
  - id: build-src
    content: pio run -e src; flash% controleren; geen File/File32-errors
    status: completed
  - id: upload-ra
    content: Alleen pio run -e src -t upload (RA). Nooit -e esp32_r4. Board niet in ESP download-mode; firmware 0.6.0 onaangeroerd.
    status: completed
  - id: native-tests-after
    content: pio test -e native opnieuw groen
    status: completed
isProject: true
---

# `[env:src]` upgraden, SdFat-clash fixen, flashen

Doel: [`platformio.ini`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/platformio.ini) `[env:src]` opschonen, de compile-fout uit [`.logs/upload_logs.txt`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/.logs/upload_logs.txt) oplossen, daarna `src/` als firmware op de **Renesas RA4M1** van de UNO R4 WiFi zetten.

## Resultaat (uitvoering)

- Native tests: 45/45 groen (voor en na).
- `platform = renesas-ra@1.9.0` → framework-arduinorenesas-uno **1.6.0**.
- SdFs/Arduino SD weg; `lib_ignore = WiFiS3, SdFs, SD`.
- `pio run -e src` SUCCESS: Flash **42.1%** (110340 / 262144 B), RAM 32.6%. Geen File/File32-errors.
- Upload: geblokkeerd — COM14 = `VID_303A` Espressif “ESP32 Family Device” (serial `DC:54:75:C3:D9:EC`), niet Arduino `VID_2341` UNO R4 WiFi. 1200bps/sam-ba en cmsis-dap falen. ESP niet geflasht (`-e esp32_r4` niet gerund).

### Upload opnieuw (handmatig)

1. USB even los/vast, of dubbel-RESET tot L-LED pulseert.
2. Controleer dat de poort als Arduino UNO R4 WiFi verschijnt (niet alleen ESP32 Family).
3. `pio run -e src -t upload` (geen `-e esp32_r4`).
