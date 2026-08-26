---
name: Wall-Z verbeteringen (index)
overview: "Master-index: Wall-Z plannen 01–10. Flash ~70% (184 KB) na Sense React + OLED clips."
todos:
  - id: plan-01
    content: "Plan 01: Bugfixes logging & gates — zie 01-bugfixes-gates-logging.plan.md"
    status: completed
  - id: plan-02
    content: "Plan 02: SD config heap & robuustheid — zie 02-sd-config-heap.plan.md"
    status: pending
  - id: plan-03
    content: "Plan 03: Features & config-profielen — zie 03-features-config-profiles.plan.md"
    status: completed
  - id: plan-04
    content: "Plan 04: Loop responsiviteit — zie 04-loop-responsiveness.plan.md"
    status: completed
  - id: plan-05
    content: "Plan 05: Build opschoning & tests — zie 05-build-cleanup-tests.plan.md"
    status: pending
  - id: plan-06
    content: "Plan 06: ESP32 IoT gateway — zie 06-esp32-iot-gateway.plan.md"
    status: completed
  - id: plan-07
    content: "Plan 07: Src env upgrade upload — zie 07-src-env-upgrade-upload.plan.md"
    status: completed
  - id: plan-08
    content: "Plan 08: Compass OLED sensors — zie 08-compass-oled-sensors.plan.md"
    status: completed
  - id: plan-09
    content: "Plan 09: Sense React autonoom (auto-menu superseded) — zie 09-sense-react-autonome.plan.md"
    status: completed
  - id: plan-10
    content: "Plan 10: Sense React UX idle OLED — zie 10-sense-react-ux-idle-oled.plan.md"
    status: completed
isProject: true
---

# Wall-Z verbeteringen — master index

Flash na STL-refactor: **39,5%** (103.596 / 262.144 B). Onderstaande 5 plannen vervangen het enkelvoudige overzicht.

## Plannen

| # | Plan | Focus | Bestand |
|---|------|-------|---------|
| **1** | Bugfixes + SD-schema | Menu logging, loop gates, **SD-tabel**, USE_LIGHT/USE_AUDIO, HM_10_BLE weg | [01-bugfixes-gates-logging.plan.md](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/.cursor/plans/01-bugfixes-gates-logging.plan.md) |
| **2** | SD config | configSaveSD zonder String, naam-parse, lazy init | [02-sd-config-heap.plan.md](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/.cursor/plans/02-sd-config-heap.plan.md) |
| **3** | Features | config.h profielen, USE_ROBOT/sensors/menu aanzetten | [03-features-config-profiles.plan.md](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/.cursor/plans/03-features-config-profiles.plan.md) |
| **4** | Responsiviteit | MicStereo non-blocking, setup delays, sonar | [04-loop-responsiveness.plan.md](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/.cursor/plans/04-loop-responsiveness.plan.md) |
| **5** | Build & tests | lib_deps trimmen, native tests uitbreiden | [05-build-cleanup-tests.plan.md](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/.cursor/plans/05-build-cleanup-tests.plan.md) |

## Aanbevolen volgorde

```mermaid
flowchart LR
  P1[Plan 1 Bugfixes] --> P2[Plan 2 SD]
  P1 --> P3[Plan 3 Features]
  P1 --> P4[Plan 4 Perf]
  P2 --> P5[Plan 5 Build]
  P4 --> P3
```

1. **Plan 1** — bugs eerst (SD-flags + menu kapot)
2. **Plan 2** — heap/SD robuustheid (menu save)
3. **Plan 3** — features aanzetten (na plan 1 verplicht voor USE_ROBOT+SD)
4. **Plan 4** — mic non-blocking vóór USE_MIC in productie
5. **Plan 5** — opruiming; kan deels parallel met 3/4

## Al afgerond (niet opnieuw)

- STL/iostream removal (flash 93% → 39,5%)
- millis + Deadline in loop-path (avoid, dance, display, PS4)
- Native tests: deadline + ps4_parse (13/13)

## Locatie

Alle plannen staan in:

`c:\workspace\projects\Arduino_Projects\Arduino-R4_UNO_Wall-Z\.cursor\plans\`
