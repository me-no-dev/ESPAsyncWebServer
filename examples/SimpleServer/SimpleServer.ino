//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#ifdef ESP32
  #include <AsyncTCP.h>
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(TARGET_RP2040)
  #include <WebServer.h>
  #include <WiFi.h>
#endif

#include <ESPAsyncWebServer.h>

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <AsyncMessagePack.h>

AsyncWebServer server(80);

const char* PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

AsyncCallbackJsonWebHandler* jsonHandler = new AsyncCallbackJsonWebHandler("/json2");
AsyncCallbackMessagePackWebHandler* msgPackHandler = new AsyncCallbackMessagePackWebHandler("/msgpack2");

void setup() {

  Serial.begin(115200);

  // WiFi.mode(WIFI_STA);
  // WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  // if (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //   Serial.printf("WiFi Failed!\n");
  //   return;
  // }
  // Serial.print("IP Address: ");
  // Serial.println(WiFi.localIP());

  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Hello, world");
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest* request) {
    String message;
    if (request->hasParam(PARAM_MESSAGE)) {
      message = request->getParam(PARAM_MESSAGE)->value();
    } else {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, GET: " + message);
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest* request) {
    String message;
    if (request->hasParam(PARAM_MESSAGE, true)) {
      message = request->getParam(PARAM_MESSAGE, true)->value();
    } else {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, POST: " + message);
  });

  // JSON

  // receives JSON and sends JSON
  jsonHandler->onRequest([](AsyncWebServerRequest* request, JsonVariant& json) {
    JsonObject jsonObj = json.as<JsonObject>();
    // ...

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot().to<JsonObject>();
    root["hello"] = "world";
    response->setLength();
    request->send(response);
  });

  // sends JSON
  server.on("/json1", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot().to<JsonObject>();
    root["hello"] = "world";
    response->setLength();
    request->send(response);
  });

  // MessagePack

  // receives MessagePack and sends MessagePack
  msgPackHandler->onRequest([](AsyncWebServerRequest* request, JsonVariant& json) {
    JsonObject jsonObj = json.as<JsonObject>();
    // ...

    AsyncMessagePackResponse* response = new AsyncMessagePackResponse();
    JsonObject root = response->getRoot().to<JsonObject>();
    root["hello"] = "world";
    response->setLength();
    request->send(response);
  });

  // sends MessagePack
  server.on("/msgpack1", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncMessagePackResponse* response = new AsyncMessagePackResponse();
    JsonObject root = response->getRoot().to<JsonObject>();
    root["hello"] = "world";
    response->setLength();
    request->send(response);
  });

  server.addHandler(jsonHandler);
  server.addHandler(msgPackHandler);

  server.onNotFound(notFound);

  server.begin();
}

void loop() {
}