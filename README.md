# T-Display-S3 Nightscout Anzeige (PlatformIO)

Diese Vorlage wurde auf eine einzige PlatformIO-Umgebung reduziert.
Sie zeigt auf dem LilyGO T-Display-S3 automatisch im 3-Sekunden-Wechsel:

1. den aktuellen Blutzucker aus Nightscout (Token-Authentifizierung)
2. die aktuelle Uhrzeit

## Struktur

- `platformio.ini` – PlatformIO-Setup für ESP32-S3 + TFT_eSPI + ArduinoJson
- `src/main.cpp` – WLAN, Nightscout-Abruf, Zeit (NTP), Display-Logik
- `include/config.h.example` – Konfigurationsvorlage für WLAN/Nightscout

## Einrichtung

1. Kopiere `include/config.h.example` nach `include/config.h`.
2. Trage WLAN, `NIGHTSCOUT_BASE_URL` und `NIGHTSCOUT_TOKEN` ein.
3. Passe ggf. `GMT_OFFSET_SECONDS` / `DAYLIGHT_OFFSET_SECONDS` an.
4. Flashen mit:

```bash
pio run -t upload
```

## Hinweis zu Nightscout

Die Abfrage erfolgt über:

`/api/v1/entries/sgv.json?count=1&token=<DEIN_TOKEN>`

Falls deine Nightscout-Instanz einen anderen Auth- oder API-Weg nutzt,
passe die URL in `src/main.cpp` entsprechend an.
