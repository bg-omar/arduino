---
name: "02 — SD config: heap & robuustheid"
overview: "configSaveSD zonder Arduino String, optioneel naam-gebaseerde load/save, en lazy SD-init voor snellere boot zonder kaart."
todos:
  - id: save-write-line
    content: "configSaveSD: writeConfigLine(key, value) helper met file.print/println"
    status: pending
  - id: save-remove-string
    content: "Verwijder 27 String objecten + F() concat keten (SD_card.cpp 225–282)"
    status: pending
  - id: parse-by-name
    content: "parseLine: key herkennen (USE_PS4) i.p.v. alleen index — flagAt(key) lookup"
    status: completed
  - id: config-table
    content: "Statische ConfigEntry tabel: key string + bool* pointer naar main::use_*"
    status: completed
  - id: lazy-init-flag
    content: "initSD lazy: ensureSdMounted() bij configLoadSD/configSaveSD; setup optioneel skippen"
    status: pending
  - id: test-sd-parse
    content: "Native test test_sd_config: parse key=value, roundtrip volgorde-onafhankelijk"
    status: pending
  - id: verify-hardware
    content: "Robot: menu save → SETUP.TXT leesbaar; reboot laadt flags correct"
    status: pending
isProject: true
---

# Plan 2 — SD config: heap-vrij & robuust

**Project:** [Arduino-R4_UNO_Wall-Z](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z)  
**Afhankelijkheden:** plan 1 (menu logging + **schema v2**: USE_LIGHT, USE_AUDIO; geen HM_10_BLE / IRREMOTE) eerst — menu roept `configSaveSD()` aan  
**Geschatte scope:** 1–2 sessies

## Doel

Enige resterende `String`-gebruik in `src/` elimineren en SD-config robuuster maken. Logging via `logger::` blijft ongewijzigd.

## Huidige situatie

[`SD_card.cpp`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/src/SD_card.cpp):

- **Load:** C-strings (`line[40]`, `strtok`, `strtol`) — goed
- **Save:** 27× `String` + concatenatie in `file.print(F(...))` — heap bij elke menu-save
- **Mapping:** index 0–26 → `main::use_*` (regels 128–154) — volgorde in `SETUP.TXT` is heilig

### Index → flag mapping (referentie)

| Index | Key (save format) | Variable |
|-------|-------------------|----------|
| 0 | USE_ADAFRUIT | main::use_adafruit |
| 1 | USE_U8G2 | main::use_u8g2 |
| … | … | … |
| 26 | USE_HM_10_BLE | main::use_hm_10_ble |

Volledige lijst: regels 225–251 (save) en 128–154 (load).

## Stap 1 — configSaveSD zonder String

### writeConfigLine helper

```cpp
static void writeConfigLine(const char* key, bool value) {
    file.print(key);
    file.print(',');
    file.print(value ? 1 : 0);
    file.print("\r\n");
}
```

### configSaveSD body

Vervang regels 225–282 door 27 opeenvolgende `writeConfigLine("USE_PS4", main::use_ps4);` etc. — **zelfde key-volgorde** als nu voor backward compatibility tot stap 2 klaar is.

**Winst:** geen heap allocatie; kortere code; PROGMEM-vriendelijk.

## Stap 2 — Naam-gebaseerde load (optioneel maar aanbevolen)

### ConfigEntry tabel

```cpp
struct ConfigEntry {
    const char* key;
    bool* flag;
};

static ConfigEntry configEntries[] = {
    {"USE_ADAFRUIT", &main::use_adafruit},
    // ... 27 entries
};
```

### parseLine uitbreiden

Huidige `parseLine` negeert key-naam (eerste `strtok` field). Nieuwe flow:

1. `key = strtok(line, ",")`
2. `valStr = strtok(nullptr, ",")`
3. `value = strtol(valStr, ...)`
4. Loop `configEntries` → `strcmp(key, entry.key) == 0` → `*entry.flag = (value == 1)`

**Voordeel:** regels in willekeurige volgorde in `SETUP.TXT`; handmatig editen veilig.

**Fallback:** onbekende key → `#if LOG_VERBOSE` log + skip (niet crashen).

### Verwijder configValue[27] index-array

Na migratie: `configCount`, `flagAt(index)` en verbose index-dump kunnen weg of alleen keys loggen.

## Stap 3 — Lazy SD init

### Probleem

[`main_ra.cpp:126`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/src/main_ra.cpp) roept altijd `SD_card::initSD()` aan → SPI probe + SdFat ook zonder kaart.

### Oplossing

```cpp
static bool sdMounted = false;

bool SD_card::ensureMounted() {
    if (sdMounted) return main::use_sd_card;
    // SD.begin(...) — zelfde als initSD nu
    sdMounted = true;
    return main::use_sd_card;
}
```

- **setup:** alleen `initSD()` als `USE_SD_CARD==1` (no-SD pad) of altijd proberen (SD-pad) — kies één policy:
  - *Conservatief:* setup blijft proberen (huidig gedrag voor SD-gebruikers)
  - *Snel:* `#if USE_SD_CARD` guard in setup; lazy mount bij eerste save/load via menu
- **configSaveSD / configLoadSD:** eerste regel `if (!ensureMounted()) return;`

Documenteer in comment: zonder SD-kaart en `USE_SD_CARD=0` → boot ~500 ms sneller (meten).

## Tests (native)

Nieuw: [`test/test_sd_config/test_sd_config.cpp`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/test/test_sd_config/test_sd_config.cpp)

Extract parse-logic naar [`src/sd_config_parse.h`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/src/sd_config_parse.h) (header-only, geen Arduino SD):

Cases:
- `"USE_PS4,1\r\n"` → key PS4, value 1
- Verkeerde volgorde: `"USE_MIC,0\r\nUSE_PS4,1\r\n"` → beide correct
- `"USE_PS4,abc"` → parse fail
- Overflow: 28+ regels → error log (simulated)

PlatformIO: `env:native` pikt test automatisch op (geen `test_filter`).

## Verificatie

1. `pio test -e native` — bestaande 13 + nieuwe SD tests
2. `grep String src/` — 0 hits
3. Handmatig: menu toggle → `SETUP.TXT` op SD leesbaar in PC
4. Reboot met SD → flags geladen, logger dump `"use_ps4: true"` etc.

## Risico's

| Risico | Mitigatie |
|--------|-----------|
| Oude SETUP.TXT met andere volgorde | Naam-parse lost op; index-only load deprecate |
| Lazy init: menu save zonder SD | `ensureMounted` faalt → logger::logln, geen crash |
| `use_sd_card` flag na failed mount | Blijft `false`; FEATURE_ENABLED valt terug op config.h |

## Niet in scope

- SD helemaal optioneel linken (plan 5 lib_deps)
- SETUP.TXT encryptie / versioning

## Bestanden

| Bestand | Actie |
|---------|-------|
| [`src/SD_card.cpp`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/src/SD_card.cpp) | Save/load refactor |
| [`src/SD_card.h`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/src/SD_card.h) | `ensureMounted()` decl |
| [`src/sd_config_parse.h`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/src/sd_config_parse.h) | Nieuw, testbaar |
| [`src/main_ra.cpp`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/src/main_ra.cpp) | Optioneel lazy init guard |
| [`test/test_sd_config/test_sd_config.cpp`](c:/workspace/projects/Arduino_Projects/Arduino-R4_UNO_Wall-Z/test/test_sd_config/test_sd_config.cpp) | Nieuw |
