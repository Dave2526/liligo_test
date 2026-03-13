#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include <Arduino_GFX_Library.h>

#include "config.h"

namespace {
constexpr int kTftBl = 38;
constexpr int kTftPower = 15;
constexpr int kTftCs = 6;
constexpr int kTftDc = 7;
constexpr int kTftRst = 5;
constexpr int kTftSclk = 18;
constexpr int kTftMosi = 17;

constexpr unsigned long kScreenSwitchMs = 3000;
constexpr unsigned long kGlucoseRefreshMs = 60000;

Arduino_DataBus* bus = new Arduino_ESP32SPI(kTftDc, kTftCs, kTftSclk, kTftMosi, GFX_NOT_DEFINED);
Arduino_GFX* gfx = new Arduino_ST7789(bus, kTftRst, 0, true, 170, 320, 35, 0, 35, 0);

unsigned long lastSwitchMs = 0;
unsigned long lastGlucoseFetchMs = 0;
bool showClock = false;

String glucoseValue = "--";
String glucoseTimestamp = "keine Daten";
bool glucoseOk = false;

void drawCentered(const String& text, int y, uint16_t color, int textSize) {
  gfx->setTextSize(textSize);
  gfx->setTextColor(color);
  const int16_t x = (gfx->width() - static_cast<int>(text.length()) * 6 * textSize) / 2;
  gfx->setCursor(max(0, x), y);
  gfx->print(text);
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  gfx->fillScreen(BLACK);
  drawCentered("Verbinde WLAN...", gfx->height() / 2 - 8, WHITE, 2);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
}

bool fetchGlucose() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  const String url = String(NIGHTSCOUT_BASE_URL) +
                     "/api/v1/entries/sgv.json?count=1&token=" +
                     String(NIGHTSCOUT_TOKEN);

  if (!http.begin(client, url)) {
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, http.getString());
  http.end();

  if (err || !doc.is<JsonArray>() || doc.size() == 0) {
    return false;
  }

  const JsonObject entry = doc[0];
  if (!entry["sgv"].is<int>()) {
    return false;
  }

  glucoseValue = String(entry["sgv"].as<int>()) + " mg/dL";
  glucoseTimestamp = entry["dateString"].is<const char*>()
                         ? String(entry["dateString"].as<const char*>())
                         : "Zeit unbekannt";
  return true;
}

String nowAsString() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    return "Zeit ?";
  }

  char buf[16];
  strftime(buf, sizeof(buf), "%H:%M:%S", &timeInfo);
  return String(buf);
}

void drawGlucoseScreen() {
  gfx->fillScreen(BLACK);
  drawCentered("Nightscout", 16, CYAN, 2);
  drawCentered(glucoseOk ? glucoseValue : "Abruf fehlgeschlagen", gfx->height() / 2 - 12,
               glucoseOk ? GREEN : RED, 2);
  drawCentered(glucoseTimestamp, gfx->height() - 20, LIGHTGREY, 1);
}

void drawClockScreen() {
  gfx->fillScreen(BLACK);
  drawCentered("Uhrzeit", 16, YELLOW, 2);
  drawCentered(nowAsString(), gfx->height() / 2 - 20, WHITE, 4);
}
}  // namespace

void setup() {
  pinMode(kTftPower, OUTPUT);
  digitalWrite(kTftPower, HIGH);

  pinMode(kTftBl, OUTPUT);
  digitalWrite(kTftBl, HIGH);

  gfx->begin();
  gfx->fillScreen(BLACK);
  drawCentered("Display OK", gfx->height() / 2 - 12, GREEN, 2);
  delay(800);

  connectWifi();
  configTime(GMT_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, "pool.ntp.org", "time.nist.gov");

  glucoseOk = fetchGlucose();
  lastGlucoseFetchMs = millis();

  drawGlucoseScreen();
  lastSwitchMs = millis();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastGlucoseFetchMs >= kGlucoseRefreshMs) {
    glucoseOk = fetchGlucose();
    lastGlucoseFetchMs = now;
    if (!showClock) {
      drawGlucoseScreen();
    }
  }

  if (now - lastSwitchMs >= kScreenSwitchMs) {
    showClock = !showClock;
    if (showClock) {
      drawClockScreen();
    } else {
      drawGlucoseScreen();
    }
    lastSwitchMs = now;
  }

  if (showClock) {
    drawClockScreen();
    delay(250);
  } else {
    delay(50);
  }
}
