/*
 * ESP32 Firebase Realtime Database Logger (REST)
 * -----------------------------------------------
 * Pushes sensor readings to Firebase Realtime Database over its plain REST
 * API - no heavyweight SDK. Each reading is POSTed under LOG_PATH with a
 * server-side timestamp ({".sv":"timestamp"}), and the upload interval is
 * read back from CONFIG_PATH so you can change it remotely from the Firebase
 * console.
 *
 * The sample reads the on-chip temperature sensor so it runs with no external
 * wiring. Replace readSensor() with your own sensor once uploads work.
 *
 * Required libraries: ArduinoJson (Benoit Blanchon).
 *   WiFi.h, WiFiClientSecure.h and HTTPClient.h ship with the ESP32 core.
 *
 * Board: any ESP32 dev board.
 *
 * Setup:
 *   1. Copy config.h.example to config.h
 *   2. Fill in WiFi creds, your database host and (optionally) an auth token
 *   3. Flash. Open Serial Monitor at 115200.
 *
 * (c) 2026 Fluidwire. Released under the MIT License.
 * https://github.com/fluidwire
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

uint32_t intervalS = 30;          // overridden by CONFIG_PATH if present
uint32_t lastUpload = 0;
uint32_t lastConfigCheck = 0;
const uint32_t CONFIG_CHECK_MS = 120000;

String buildUrl(const char *path) {
  String url = "https://";
  url += FIREBASE_HOST;
  url += path;
  url += ".json";
  if (strlen(FIREBASE_AUTH) > 0) {
    url += "?auth=";
    url += FIREBASE_AUTH;
  }
  return url;
}

void connectWiFi() {
  Serial.printf("WiFi: connecting to %s ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 20000) {
      Serial.println("FAILED, restarting");
      ESP.restart();
    }
    delay(500);
    Serial.print(".");
  }
  Serial.printf(" OK, ip=%s\n", WiFi.localIP().toString().c_str());
}

// Placeholder sensor: on-chip temperature. Swap for your own.
float readSensor() {
  return temperatureRead();
}

// POST one reading; Firebase returns {"name":"<push-id>"} on success.
bool pushReading(float value) {
  WiFiClientSecure client;
  client.setInsecure();  // dev only - pin the Google root CA for production
  HTTPClient http;
  if (!http.begin(client, buildUrl(LOG_PATH))) {
    Serial.println("HTTP begin() failed (check FIREBASE_HOST)");
    return false;
  }
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["value"] = value;
  doc["uptime_s"] = millis() / 1000;
  doc["ts"][".sv"] = "timestamp";   // server-side timestamp (ms since epoch)
  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  if (code == 200) {
    Serial.printf("POST -> 200 %s\n", http.getString().c_str());
  } else if (code == 401) {
    Serial.println("POST -> 401 (auth token wrong or rules deny writes)");
  } else if (code == 404) {
    Serial.println("POST -> 404 (check FIREBASE_HOST)");
  } else if (code < 0) {
    Serial.printf("POST failed: %s\n", http.errorToString(code).c_str());
  } else {
    Serial.printf("POST -> %d %s\n", code, http.getString().c_str());
  }
  http.end();
  return code == 200;
}

// GET the remote-config interval. A bare number such as 30 comes back as "30".
void fetchConfig() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, buildUrl(CONFIG_PATH))) return;
  int code = http.GET();
  if (code == 200) {
    long v = http.getString().toInt();
    if (v >= 5 && v <= 3600) {
      if ((uint32_t)v != intervalS) Serial.printf("Config: interval -> %ld s\n", v);
      intervalS = (uint32_t)v;
    }
  } else {
    Serial.printf("Config GET -> %d (keeping %lu s)\n", code, (unsigned long)intervalS);
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nFluidwire ESP32 Firebase RTDB logger");
  connectWiFi();
  fetchConfig();
  lastConfigCheck = millis();
  lastUpload = millis() - intervalS * 1000UL;  // upload right away
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  if (millis() - lastConfigCheck >= CONFIG_CHECK_MS) {
    lastConfigCheck = millis();
    fetchConfig();
  }
  if (millis() - lastUpload >= intervalS * 1000UL) {
    lastUpload = millis();
    pushReading(readSensor());
  }
  delay(50);
}
