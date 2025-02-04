// Lightweight and highly customizable "WiFi manager"
//
// Non-blocking WiFi handling with captive portal for ESP32
// Will try to reconnect to a saved network automatically when disconnected
// 
// A combination of
// https://github.com/copercini/esp32-iot-examples/blob/master/WiFi_portal/WiFi_portal.ino
// &
// https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/examples/CaptivePortal/CaptivePortal.ino

#include <Arduino.h>

#include <Preferences.h>
#include <DNSServer.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#include "ESPAsyncWebServer.h"

char AP_SSID[30];

DNSServer dnsServer;
AsyncWebServer server(80);

Preferences preferences;
static volatile bool wifi_connected = false;
static volatile bool ap_on = false;
String wifiSSID, wifiPassword;


class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request) {
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    switch(request->method()) {
      case HTTP_GET:
        if(request->hasParam("ssid") && request->hasParam("pass")) {
          AsyncWebParameter * ssid = request->getParam("ssid");
          wifiSSID = ssid->value();
          Serial.printf("ssid: %s\n", ssid->value().c_str());

          AsyncWebParameter * pass = request->getParam("pass");
          wifiPassword = pass->value();
          Serial.printf("pass: %s\n", pass->value().c_str());
        
          preferences.begin("wifi", false);
          Serial.println("Writing new ssid");
          preferences.putString("ssid", ssid->value().c_str());

          Serial.println("Writing new pass");
          preferences.putString("password", pass->value().c_str());
          delay(50);
          preferences.end();

          AsyncResponseStream *response = request->beginResponseStream("text/html");
          response->print("<h1>OK! Trying to connect to new WiFi...</h1>");
          response->print("<h2>This portal will be closed shortly if connection is successful</h2>");
          request->send(response);
          
          Serial.println("Trying to connect...");
          WiFi.begin(ssid->value().c_str(), pass->value().c_str());
        }

        else {
          AsyncResponseStream *response = request->beginResponseStream("text/html");
          response->print("<!DOCTYPE html><html><head><title>Captive portal for entering WiFi credentials</title></head><body>");
          response->print("<form action='/' method='GET'><label>SSID: </label><input id='ssid' name='ssid' length=32><input id='pass' name='pass' length=64><input type='submit' value='submit'></form>");
          response->print("</body></html>");
          request->send(response);
        }
        
      break;
    }
  }
};

/* When WiFi connects */
void wifiOnConnect() {
  Serial.println("STA Connected");
  Serial.print("STA SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("STA IPv4: ");
  Serial.println(WiFi.localIP());
  WiFi.mode(WIFI_MODE_STA);
  ap_on = false;
}

/* When WiFi disconnects */
void wifiOnDisconnect() {
  Serial.println("STA Disconnected");
  
  if(!ap_on) {
    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.softAP(AP_SSID);
    Serial.println("AP Started");
    Serial.print("AP SSID: ");
    Serial.println(AP_SSID);
    Serial.print("AP IPv4: ");
    Serial.println(WiFi.softAPIP());
    ap_on = true;
  }
  
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
}

/* WiFi events */
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_AP_START:
      WiFi.softAPsetHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_STA_START:
      WiFi.setHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      wifiOnConnect();
      wifi_connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      wifi_connected = false;
      wifiOnDisconnect();
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  snprintf(AP_SSID, 30, "ESP-%04X", (uint16_t)ESP.getEfuseMac());

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_APSTA);
  ap_on = true;
  WiFi.softAP(AP_SSID);
  
  Serial.println("AP Started");
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IPv4: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);

  preferences.begin("wifi", false);
  wifiSSID = preferences.getString("ssid", "none");
  wifiPassword = preferences.getString("password", "none");
  preferences.end();
  Serial.print("Stored SSID: ");
  Serial.println(wifiSSID);

  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  server.begin();
}

void loop() {
  if(wifi_connected) {
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    delay(1000);
  }

  dnsServer.processNextRequest();
}