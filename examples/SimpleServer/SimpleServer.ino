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
#include <LittleFS.h>

AsyncWebServer server(80);
AsyncEventSource events("/events");

const char* PARAM_MESSAGE PROGMEM = "message";
const char* SSE_HTLM PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Server-Sent Events</title>
  <script>
    if (!!window.EventSource) {
      var source = new EventSource('/events');
      source.addEventListener('open', function(e) {
        console.log("Events Connected");
      }, false);
      source.addEventListener('error', function(e) {
        if (e.target.readyState != EventSource.OPEN) {
          console.log("Events Disconnected");
        }
      }, false);
      source.addEventListener('message', function(e) {
        console.log("message", e.data);
      }, false);
      source.addEventListener('heartbeat', function(e) {
        console.log("heartbeat", e.data);
      }, false);
    }
  </script>
</head>
<body>
  <h1>Open your browser console!</h1>
</body>
</html>
)";

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

AsyncCallbackJsonWebHandler* jsonHandler = new AsyncCallbackJsonWebHandler("/json2");
AsyncCallbackMessagePackWebHandler* msgPackHandler = new AsyncCallbackMessagePackWebHandler("/msgpack2");

void setup() {

  Serial.begin(115200);

#ifndef CONFIG_IDF_TARGET_ESP32H2
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
#endif

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Hello, world");
  });

  server.on("/file", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/index.html");
  });

  /*
    ❯ curl -I -X HEAD http://192.168.4.1/download
    HTTP/1.1 200 OK
    Content-Length: 1024
    Content-Type: application/octet-stream
    Connection: close
    Accept-Ranges: bytes
  */
  // Ref: https://github.com/mathieucarbou/ESPAsyncWebServer/pull/80
  server.on("/download", HTTP_HEAD | HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->method() == HTTP_HEAD) {
      AsyncWebServerResponse* response = request->beginResponse(200, "application/octet-stream");
      response->addHeader(asyncsrv::T_Accept_Ranges, "bytes");
      response->addHeader(asyncsrv::T_Content_Length, 10);
      response->setContentLength(1024); // overrides previous one
      response->addHeader(asyncsrv::T_Content_Type, "foo");
      response->setContentType("application/octet-stream"); // overrides previous one
      // ...
      request->send(response);
    } else {
      // ...
    }
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

  events.onConnect([](AsyncEventSourceClient* client) {
    if (client->lastId()) {
      Serial.printf("SSE Client reconnected! Last message ID that it gat is: %" PRIu32 "\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 1000);
  });

  server.on("/sse", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", SSE_HTLM);
  });

  server.addHandler(&events);
  server.addHandler(jsonHandler);
  server.addHandler(msgPackHandler);

  server.onNotFound(notFound);

  server.begin();
}

uint32_t lastSSE = 0;
uint32_t delta = 5;

void loop() {
  uint32_t now = millis();
  if (now - lastSSE >= delta) {
    events.send(String("ping-") + now, "heartbeat", now);
    lastSSE = millis();
  }
}