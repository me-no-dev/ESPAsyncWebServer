/*
  This is an example of a simple file based web server. It can automatically serve gziped/.gz files.

  Uses LittleFS.

  Holger Lembke / lembke@gmail.com 2021
*/

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#include <FS.h>
#include <LittleFS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

const char* ssid = "....";
const char* password = "....";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  if (! LittleFS.begin(false)) { // change to true if you want to force formatting
    Serial.println("LittleFS Mount Failed, no forced formatting.");
  }

  // this is it!
  setupWebserver();
}

void loop() {
  // empty!
}
