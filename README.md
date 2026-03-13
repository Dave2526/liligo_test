# T-Display-S3 Nightscout (PlatformIO)

Diese Version orientiert sich enger an typischen T-Display-S3-Templates mit
`Arduino_GFX` und expliziter Display-/Power-Pin-Initialisierung.

## Anzeige

Automatischer Wechsel alle 3 Sekunden zwischen:

1. Nightscout-Blutzucker (Token-Auth)
2. Uhrzeit (NTP)

## Konfiguration

1. `include/config.h.example` nach `include/config.h` kopieren.
2. WLAN + Nightscout URL + Token eintragen.
3. Zeitzone über `GMT_OFFSET_SECONDS` / `DAYLIGHT_OFFSET_SECONDS` setzen.

## Flashen

```bash
pio run -t upload
```

## Verwendete Display-Pins (template-nah)

- `TFT_CS=6`
- `TFT_DC=7`
- `TFT_RST=5`
- `TFT_SCLK=18`
- `TFT_MOSI=17`
- `TFT_BL=38`
- `TFT_POWER=15`
