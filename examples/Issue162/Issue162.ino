/**
 *
 * Connect to AP and run in bash:
 *
 * > while true; do echo "PING"; sleep 0.1; done | websocat ws://192.168.4.1/ws
 *
 */
#include <Arduino.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#endif
#ifdef ESP32
  #include <WiFi.h>
#endif
#include <ESPAsyncWebServer.h>

#include <list>
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Client #%" PRIu32 " connected.\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Client #%" PRIu32 " disconnected.\n", client->id());
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("Client #%" PRIu32 " error.\n", client->id());
  } else if (type == WS_EVT_DATA) {
    Serial.printf("Client #%" PRIu32 " len: %u\n", client->id(), len);
  } else if (type == WS_EVT_PONG) {
    Serial.printf("Client #%" PRIu32 " pong.\n", client->id());
  } else if (type == WS_EVT_PING) {
    Serial.printf("Client #%" PRIu32 " ping.\n", client->id());
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/close_all_ws_clients", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest* request) {
    Serial.println("Closing all WebSocket clients...");
    ws.closeAll();
    request->send(200, "application/json", "{\"status\":\"all clients closed\"}");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
        <html>
        <head></head>
        <script> 
            let ws = new WebSocket("ws://" + window.location.host + "/ws");
                    
            document.addEventListener("DOMContentLoaded", function () {
                ws.onopen = function () {
                    console.log("WebSocket connected");
                    };
                });
                        
            function closeAllWsClients() {
              fetch("/close_all_ws_clients", {
                  method : "POST",
              });
            };
        </script>
        <body>
            <button onclick = "closeAllWsClients()" style = "width: 200px"> ws close all</button><p></p>  
          </body>
        </html>
  )rawliteral");
  });

  server.begin();
}

void loop() {
  vTaskDelete(NULL);
}
