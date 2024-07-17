#include <DNSServer.h>
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

#include "ESPAsyncWebServer.h"

const char appWebPage[] PROGMEM = R"rawliteral(
<body>
<button id="button1" onclick="fetch('/button1');">Relay1</button>
<script>
  const evtSource = new EventSource("/events");
  button1 = document.getElementById("button1");
  evtSource.addEventListener('state', (e) => {
      const data = JSON.parse(e.data);
      console.log('Event Source data: ', data);
      if (data.button1) {
          button1.style.backgroundColor = "green";
      }
      else {
          button1.style.backgroundColor = "red";
      }
  });
</script>
</body>
)rawliteral";

AsyncWebServer server(80);
AsyncEventSource events("/events");

const uint32_t interval = 1000;
const int button1Pin = 4;

uint32_t lastSend = 0;

void prepareJson(String& buffer) {
  buffer.reserve(512);
  buffer.concat("{\"button1\":");
  buffer.concat(digitalRead(button1Pin) == LOW);
  buffer.concat(",\"1234567890abcdefghij1234567890abcdefghij1234567890abcdefghij1234567890abcdefghij1234567890abcdefghij1234567890abcdefghij\":");
  buffer.concat(random(0, 999999999));
  buffer.concat("}");
}

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);
  delay(100);
#else
  while (!Serial)
    yield();
#endif

  randomSeed(micros());

  pinMode(button1Pin, OUTPUT);
  digitalWrite(button1Pin, HIGH);

  WiFi.softAP("esp-captive");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", appWebPage);
  });

  server.on("/button1", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "OK");
    digitalWrite(button1Pin, digitalRead(button1Pin) == LOW ? HIGH : LOW);

    String buffer;
    prepareJson(buffer);
    ESP_LOGI("async_tcp", "Sending from handler...");
    events.send(buffer.c_str(), "state", millis());
    ESP_LOGI("async_tcp", "Sent from handler!");
  });

  events.onConnect([](AsyncEventSourceClient* client) {
    String buffer;
    prepareJson(buffer);
    ESP_LOGI("async_tcp", "Sending from onConnect...");
    client->send(buffer.c_str(), "state", millis(), 5000);
    ESP_LOGI("async_tcp", "Sent from onConnect!");
  });

  server.addHandler(&events);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.begin();
}

void loop() {
  if (millis() - lastSend >= interval) {
    String buffer;
    prepareJson(buffer);
    ESP_LOGI("loop", "Sending...");
    events.send(buffer.c_str(), "state", millis());
    ESP_LOGI("loop", "Sent!");
    lastSend = millis();
  }
}
