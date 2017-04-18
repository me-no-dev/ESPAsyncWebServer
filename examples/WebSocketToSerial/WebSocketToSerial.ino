#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "**********";
const char* password = "************";

String inputString = "";

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    char buff[3];
    String str = "";

    // Grab all data
    for(size_t i=0; i < info->len; i++) {
      if (info->opcode == WS_TEXT ) {
        str += (char) data[i];
      } else {
        sprintf(buff, "%02x ", (uint8_t) data[i]);
        str += buff ;
      }
    }

    if (str == "ping")
      client->printf("pong");
    else if (str!="") {
      Serial.print(info->opcode==WS_TEXT?"Text:":"Binary 0x:");
      Serial.println(str);
    }
  }
}

void setup(){
  Serial.begin(115200);
  Serial.println(F("\r\nWebSocket2Serial"));
  SPIFFS.begin();
  WiFi.mode(WIFI_STA);
  // empty sketch SSID, try autoconnect with SDK saved credentials
  if (*ssid!='*' && *password!='*' ) {
    Serial.printf("connecting to %s with psk %s\r\n", ssid, password );
    WiFi.begin(ssid, password);
  } else {
    Serial.println(F("No SSID/PSK defined in sketch\r\nConnecting with SDK ones if any"));
  }

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("Failed to connect, retrying!"));
    WiFi.disconnect(false);
    delay(2000);
    // empty sketch SSID, try autoconnect with SDK saved crededntials
    if (*ssid!='*' && *password!='*' ) {
      WiFi.begin(ssid, password);
    }
  }
  Serial.print(F("Connected with "));
  Serial.println(WiFi.localIP());
  ArduinoOTA.begin();
  //Serial.printf("format start\n"); SPIFFS.format();  Serial.printf("format end\n");

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.serveStatic("/", SPIFFS, "/index.htm", "max-age=86400");
  server.serveStatic("/", SPIFFS, "/",          "max-age=86400"); 

  server.onNotFound([](AsyncWebServerRequest *request){
    os_printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      os_printf("GET");
    else if(request->method() == HTTP_POST)
      os_printf("POST");
    else if(request->method() == HTTP_DELETE)
      os_printf("DELETE");
    else if(request->method() == HTTP_PUT)
      os_printf("PUT");
    else if(request->method() == HTTP_PATCH)
      os_printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      os_printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      os_printf("OPTIONS");
    else
      os_printf("UNKNOWN");

    os_printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      os_printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      os_printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      os_printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        os_printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        os_printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        os_printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  // Start Server
  server.begin();
}

void loop() {
  if (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;

    if (inChar == '\n') {
      ws.textAll(inputString);
      inputString = "";
    }
  }

  ArduinoOTA.handle();
}
