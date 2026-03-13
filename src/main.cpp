#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "config.h"

namespace {
constexpr int kBacklightPin = TFT_BL;
constexpr unsigned long kScreenSwitchMs = 3000;
constexpr unsigned long kGlucoseRefreshMs = 60000;

TFT_eSPI tft;
unsigned long lastSwitchMs = 0;
unsigned long lastGlucoseFetchMs = 0;
bool showClock = false;

String glucoseValue = "--";
String glucoseTimestamp = "keine Daten";
bool glucoseOk = false;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Verbinde WLAN...", tft.width() / 2, tft.height() / 2, 2);

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
  if (code <= 0) {
    http.end();
    return false;
  }

  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  const auto err = deserializeJson(doc, payload);
  if (err || !doc.is<JsonArray>() || doc.size() == 0) {
    return false;
  }

  const JsonObject entry = doc[0];
  if (!entry["sgv"].is<int>()) {
    return false;
  }

  glucoseValue = String(entry["sgv"].as<int>()) + " mg/dL";
  glucoseTimestamp = entry["dateString"].is<const char *>()
                         ? String(entry["dateString"].as<const char *>())
                         : "Zeit unbekannt";

  return true;
}

String nowAsString() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    return "Zeit nicht verfuegbar";
  }

  char buf[16];
  strftime(buf, sizeof(buf), "%H:%M:%S", &timeInfo);
  return String(buf);
}

void drawGlucoseScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Nightscout", tft.width() / 2, 40, 4);

  tft.setTextColor(glucoseOk ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(glucoseOk ? glucoseValue : "Abruf fehlgeschlagen", tft.width() / 2,
                 tft.height() / 2, 4);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(glucoseTimestamp, tft.width() / 2, tft.height() - 24, 2);
}

void drawClockScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Uhrzeit", tft.width() / 2, 40, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(nowAsString(), tft.width() / 2, tft.height() / 2, 7);
}
}  // namespace

void setup() {
  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Display OK", tft.width() / 2, tft.height() / 2, 4);
  delay(600);

  connectWifi();
  configTime(GMT_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, "pool.ntp.org",
             "time.nist.gov");

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
