# ESPAsyncWebServer [![Build Status](https://travis-ci.org/me-no-dev/ESPAsyncWebServer.svg?branch=master)](https://travis-ci.org/me-no-dev/ESPAsyncWebServer)

For help and support [![Join the chat at https://gitter.im/me-no-dev/ESPAsyncWebServer](https://badges.gitter.im/me-no-dev/ESPAsyncWebServer.svg)](https://gitter.im/me-no-dev/ESPAsyncWebServer?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Async HTTP and WebSocket Server for ESP8266 and ESP31B Arduino

Requires [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) to work

To use this library you need to have the latest git versions of either [ESP8266](https://github.com/esp8266/Arduino) or [ESP31B](https://github.com/me-no-dev/ESP31B) Arduino Core

## Why should you care
- Using asynchronous network means that you can handle more than one connection at the same time
- You are called once the request is ready and parsed
- When you send the response, you are immediately ready to handle other connections
  while the server is taking care of sending the response in the background
- Speed is OMG
- Easy to use API, HTTP Basic Authentication, ChunkedResponse
- Easily extendible to handle any type of content
- Supports Continue 100
- Async WebSocket plugin offering different locations without extra servers or ports

## Important things to remember
- This is fully asynchronous server and as such does not run on the loop thread.
- You can not use yield or delay or any function that uses them inside the callbacks
- The server is smart enough to know when to close the connection and free resources
- You can not send more than one response to a single request

## Principles of operation

### The Async Web server
- Listens for connections
- Wraps the new clients into ```Request```
- Keeps track of clients and cleans memory
- Manages ```Handlers``` and attaches them to Requests

### Request Life Cycle
- TCP connection is received by the server
- The connection is wrapped inside ```Request``` object
- When the request head is received (type, url, get params, http version and host),
  the server goes through all attached ```Handlers```(in the order they are attached) trying to find one
  that ```canHandle``` the given request. If none are found, the default(catch-all) handler is attached.
- The rest of the request is received, calling the ```handleUpload``` or ```handleBody``` methods of the ```Handler``` if they are needed (POST+File/Body)
- When the whole request is parsed, the result is given to the ```handleRequest``` method of the ```Handler``` and is ready to be responded to
- In the ```handleRequest``` method, to the ```Request``` is attached a ```Response``` object (see below) that will serve the response data back to the client
- When the ```Response``` is sent, the client is closed and freed from the memory

### Handlers and how do they work
- The ```Handlers``` are used for executing specific actions to particular requests
- One ```Handler``` instance can be attached to any request and lives together with the server
- The ```canHandle``` method is used for filtering the requests that can be handled
  and declaring any interesting headers that the ```Request``` should collect 
- Decision can be based on request method, request url, http version, request host/port/target host and GET parameters
- Once a ```Handler``` is attached to given ```Request``` (```canHandle``` returned true)
  that ```Handler``` takes care to receive any file/data upload and attach a ```Response```
  once the ```Request``` has been fully parsed
- ```Handler's``` ```canHandle``` is called in the order they are attached to the server.
  If a ```Handler``` attached earlier returns ```true``` on ```canHandle```, then this ```Hander's``` ```canHandle``` will never be called

### Responses and how do they work
- The ```Response``` objects are used to send the response data back to the client
- The ```Response``` object lives with the ```Request``` and is freed on end or disconnect
- Different techniques are used depending on the response type to send the data in packets
  returning back almost immediately and sending the next packet when this one is received.
  Any time in between is spent to run the user loop and handle other network packets
- Responding asynchronously is probably the most difficult thing for most to understand
- Many different options exist for the user to make responding a background task


## Request Variables

### Common Variables
```cpp
request->version();       // uint8_t: 0 = HTTP/1.0, 1 = HTTP/1.1
request->method();        // enum:    HTTP_GET, HTTP_POST, HTTP_DELETE, HTTP_PUT, HTTP_PATCH, HTTP_HEAD, HTTP_OPTIONS
request->url();           // String:  URL of the request (not including host, port or GET parameters) 
request->host();          // String:  The requested host (can be used for virtual hosting)
request->contentType();   // String:  ContentType of the request (not avaiable in Handler::canHandle)
request->contentLength(); // size_t:  ContentLength of the request (not avaiable in Handler::canHandle)
request->multipart();     // bool:    True if the request has content type "multipart"
```

### Headers
```cpp
//List all collected headers
int headers = request->headers();
int i;
for(i=0;i<headers;i++){
  AsyncWebHeader* h = request->getHeader(i);
  Serial.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
}

//get specific header by name
if(request->hasHeader("MyHeader")){
  AsyncWebHeader* h = request->getHeader("MyHeader");
  Serial.printf("MyHeader: %s\n", h->value().c_str());
}
```

### GET, POST and FILE parameters
```cpp
//List all parameters
int params = request->params();
for(int i=0;i<params;i++){
  AsyncWebParameter* p = request->getParam(i);
  if(p->isFile()){ //p->isPost() is also true
    Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
  } else if(p->isPost()){
    Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
  } else {
    Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
  }
}

//Check if GET parameter exists
bool exists = request->hasParam("download");
AsyncWebParameter* p = request->getParam("download");

//Check if POST (but not File) parameter exists
bool exists = request->hasParam("download", true);
AsyncWebParameter* p = request->getParam("download", true);

//Check if FILE was uploaded
bool exists = request->hasParam("download", true, true);
AsyncWebParameter* p = request->getParam("download", true, true);
```

### FILE Upload handling
```cpp
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }
  for(size_t i=0; i<len; i++){
    Serial.write(data[i]);
  }
  if(final){
    Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index+len);
  }
}
```

### Body data handling
```cpp
void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  if(!index){
    Serial.printf("BodyStart: %u B\n", total);
  }
  for(size_t i=0; i<len; i++){
    Serial.write(data[i]);
  }
  if(index + len == total){
    Serial.printf("BodyEnd: %u B\n", total);
  }
}
```

## Responses

### Basic response with HTTP Code
```cpp
request->send(404); //Sends 404 File Not Found
```

### Basic response with HTTP Code and extra headers
```cpp
AsyncWebServerResponse *response = request->beginResponse(404); //Sends 404 File Not Found
response->addHeader("Server","ESP Async Web Server");
request->send(response);
```

### Basic response with string content
```cpp
request->send(200, "text/plain", "Hello World!");
```

### Basic response with string content and extra headers
```cpp
AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Hello World!");
response->addHeader("Server","ESP Async Web Server");
request->send(response);
```

### Respond with content coming from a Stream
```cpp
//read 12 bytes from Serial and send them as Content Type text/plain
request->send(Serial, "text/plain", 12);
```

### Respond with content coming from a Stream and extra headers
```cpp
//read 12 bytes from Serial and send them as Content Type text/plain
AsyncWebServerResponse *response = request->beginResponse(Serial, "text/plain", 12);
response->addHeader("Server","ESP Async Web Server");
request->send(response);
```

### Respond with content coming from a File
```cpp
//Send index.htm with default content type
request->send(SPIFFS, "/index.htm");

//Send index.htm as text
request->send(SPIFFS, "/index.htm", "text/plain");

//Download index.htm
request->send(SPIFFS, "/index.htm", String(), true);
```

### Respond with content coming from a File and extra headers
```cpp
//Send index.htm with default content type
AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.htm");

//Send index.htm as text
AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.htm", "text/plain");

//Download index.htm
AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.htm", String(), true);

response->addHeader("Server","ESP Async Web Server");
request->send(response);
```

### Respond with content using a callback
```cpp
//send 128 bytes as plain text
request->send("text/plain", 128, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //index equals the amount of bytes that have been already sent
  //You will not be asked for more bytes once the content length has been reached.
  //Keep in mind that you can not delay or yield waiting for more data!
  //Send what you currently have and you will be asked for more again
  return mySource.read(buffer, maxLen);
});
```

### Respond with content using a callback and extra headers
```cpp
//send 128 bytes as plain text
AsyncWebServerResponse *response = request->beginResponse("text/plain", 128, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //index equals the amount of bytes that have been already sent
  //You will not be asked for more bytes once the content length has been reached.
  //Keep in mind that you can not delay or yield waiting for more data!
  //Send what you currently have and you will be asked for more again
  return mySource.read(buffer, maxLen);
});
response->addHeader("Server","ESP Async Web Server");
request->send(response);
```

### Chunked Response
Used when content length is unknown. Works best if the client supports HTTP/1.1
```cpp
AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //index equals the amount of bytes that have been already sent
  //You will be asked for more data until 0 is returned
  //Keep in mind that you can not delay or yield waiting for more data!
  return mySource.read(buffer, maxLen);
});
response->addHeader("Server","ESP Async Web Server");
request->send(response);
```

### Print to response
```cpp
AsyncResponseStream *response = request->beginResponseStream("text/html");
response->addHeader("Server","ESP Async Web Server");
response->printf("<!DOCTYPE html><html><head><title>Webpage at %s</title></head><body>", request->url().c_str());

response->print("<h2>Hello ");
response->print(request->client()->remoteIP());
response->print("</h2>");

response->print("<h3>General</h3>");
response->print("<ul>");
response->printf("<li>Version: HTTP/1.%u</li>", request->version());
response->printf("<li>Method: %s</li>", request->methodToString());
response->printf("<li>URL: %s</li>", request->url().c_str());
response->printf("<li>Host: %s</li>", request->host().c_str());
response->printf("<li>ContentType: %s</li>", request->contentType().c_str());
response->printf("<li>ContentLength: %u</li>", request->contentLength());
response->printf("<li>Multipart: %s</li>", request->multipart()?"true":"false");
response->print("</ul>");

response->print("<h3>Headers</h3>");
response->print("<ul>");
int headers = request->headers();
for(int i=0;i<headers;i++){
  AsyncWebHeader* h = request->getHeader(i);
  response->printf("<li>%s: %s</li>", h->name().c_str(), h->value().c_str());
}
response->print("</ul>");

response->print("<h3>Parameters</h3>");
response->print("<ul>");
int params = request->params();
for(int i=0;i<params;i++){
  AsyncWebParameter* p = request->getParam(i);
  if(p->isFile()){
    response->printf("<li>FILE[%s]: %s, size: %u</li>", p->name().c_str(), p->value().c_str(), p->size());
  } else if(p->isPost()){
    response->printf("<li>POST[%s]: %s</li>", p->name().c_str(), p->value().c_str());
  } else {
    response->printf("<li>GET[%s]: %s</li>", p->name().c_str(), p->value().c_str());
  }
}
response->print("</ul>");

response->print("</body></html>");
//send the response last
request->send(response);
```

### Send a large webpage from PROGMEM using callback response
Example provided by [@nouser2013](https://github.com/nouser2013)
```cpp
const char indexhtml[] PROGMEM = "..."; // large char array, tested with 5k
AsyncWebServerResponse *response = request->beginResponse(
  String("text/html"),
  strlen_P(indexhtml),
  [](uint8_t *buffer, size_t maxLen, size_t alreadySent) -> size_t {
    if (strlen_P(indexhtml+alreadySent)>maxLen) {
      // We have more to read than fits in maxLen Buffer
      memcpy_P((char*)buffer, indexhtml+alreadySent, maxLen);
      return maxLen;
    }
    // Ok, last chunk
    memcpy_P((char*)buffer, indexhtml+alreadySent, strlen_P(indexhtml+alreadySent));
    return strlen_P(indexhtml+alreadySent); // Return from here to end of indexhtml
  }
);
response->addHeader("Server", "MyServerString");
request->send(response);  
```

### ArduinoJson Basic Response
This way of sending Json is great for when the result is below 4KB 
```cpp
#include "AsyncJson.h"
#include "ArduinoJson.h"


AsyncResponseStream *response = request->beginResponseStream("text/json");
DynamicJsonBuffer jsonBuffer;
JsonObject &root = jsonBuffer.createObject();
root["heap"] = ESP.getFreeHeap();
root["ssid"] = WiFi.SSID();
root.printTo(*response);
request->send(response);
```

### ArduinoJson Advanced Response
This response can handle really large Json objects (tested to 40KB)
There isn't any noticeable speed decrease for small results with the method above
Since ArduinoJson does not allow reading parts of the string, the whole Json has to
be passed every time a chunks needs to be sent, which shows speed decrease proportional
to the resulting json packets
```cpp
#include "AsyncJson.h"
#include "ArduinoJson.h"


AsyncJsonResponse * response = new AsyncJsonResponse();
response->addHeader("Server","ESP Async Web Server");
JsonObject& root = response->getRoot();
root["heap"] = ESP.getFreeHeap();
root["ssid"] = WiFi.SSID();
response->setLength();
request->send(response);
```

## Bad Responses
Some responses are implemented, but you should not use them, because they do not conform to HTTP.
The following example will lead to unclean close of the connection and more time wasted
than providing the length of the content

### Respond with content using a callback without content length to HTTP/1.0 clients
```cpp
//This is used as fallback for chunked responses to HTTP/1.0 Clients
request->send("text/plain", 0, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //You will be asked for more data until 0 is returned
  //Keep in mind that you can not delay or yield waiting for more data!
  return mySource.read(buffer, maxLen);
});
```

## Async WebSocket Plugin
The server includes a web socket plugin which lets you define different WebSocket locations to connect to
without starting another listening service or using different port

### Async WebSocket Event
```cpp

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //client connected
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //client disconnected
    os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //error was received from the other end
    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if(info->opcode == WS_TEXT){
        data[len] = 0;
        os_printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < info->len; i++){
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }
      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->message_opcode == WS_TEXT){
        data[len] = 0;
        os_printf("%s\n", (char*)data);
      } else {
        for(size_t i=0; i < len; i++){
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }

      if((info->index + len) == info->len){
        os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}
```

### Methods for sending data to a socket client
```cpp



//Server methods
AsyncWebSocket ws("/ws");
//printf to a client
ws.printf([client id], [arguments...]);
//printf to all clients
ws.printfAll([arguments...]);
//send text to a client
ws.text([client id], [(char*)text]);
ws.text([client id], [text], [len]);
const char flash_text[] PROGMEM = "Text to send"
ws.text([client id], [PSTR("text")]);
ws.text([client id], [FPSTR(flash_text)]);
//send text to all clients
ws.textAll([(char*text]);
ws.textAll([text], [len]);
//send binary to a client
ws.binary([client id], [(char*)binary]);
ws.binary([client id], [binary], [len]);
const uint8_t flash_binary[] PROGMEM = { 0x01, 0x02, 0x03, 0x04 };
ws.binary([client id], [flash_binary], [len]);
//send binary to all clients
ws.binaryAll([(char*binary]);
ws.binaryAll([binary], [len]);

//client methods
AsyncWebSocketClient * client;
//printf to a client
client->printf([arguments...]);
//send text to a client
client->text([(char*)text]);
client->text([text], [len]);
const char flash_text[] PROGMEM = "Text to send";
client->text([PSTR("text")]);
client->text([FPSTR(flash_text)]);
//send binary to a client
client->binary([(char*)binary]);
client->binary([binary], [len]);
const uint8_t flash_binary[] PROGMEM = { 0x01, 0x02, 0x03, 0x04 };
client->binary([flash_binary], [len]);
```


## Setting up the server
```cpp
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

const char* ssid = "your-ssid";
const char* password = "your-pass";
const char* http_username = "admin";
const char* http_password = "admin";

void onRequest(AsyncWebServerRequest *request){
  //Handle Unknown Request
  request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  //Handle body
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  //Handle upload
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  //Handle WebSocket event
}

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  
  // attach AsyncWebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  
  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  
  // upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200);
  }, handleUpload);
  
  // send a file when /index is requested
  server.on("/index", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.htm");
  });
  
  // HTTP basic authentication
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
        return request->requestAuthentication();
    request->send(200, "text/plain", "Login Success!");
  });
  
  // attach filesystem root at URL /fs 
  server.serveStatic("/fs", SPIFFS, "/");
  
  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);
  
  server.begin();
}

void loop(){}
```


