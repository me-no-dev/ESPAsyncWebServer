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
- Easy to use API, HTTP Basic and Digest MD5 Authentication (default), ChunkedResponse
- Easily extendible to handle any type of content
- Supports Continue 100
- Async WebSocket plugin offering different locations without extra servers or ports
- Async EventSource (Server-Sent Events) plugin to send events to the browser
- URL Rewrite plugin for conditional and permanent url rewrites
- ServeStatic plugin that supports cache, Last-Modified, default index and more

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
- Manages ```Rewrites``` and apply them on the request url
- Manages ```Handlers``` and attaches them to Requests

### Request Life Cycle
- TCP connection is received by the server
- The connection is wrapped inside ```Request``` object
- When the request head is received (type, url, get params, http version and host),
  the server goes through all ```Rewrites``` (in the order they were added) to rewrite the url and inject query parameters,
  next, it goes through all attached ```Handlers```(in the order they were added) trying to find one
  that ```canHandle``` the given request. If none are found, the default(catch-all) handler is attached.
- The rest of the request is received, calling the ```handleUpload``` or ```handleBody``` methods of the ```Handler``` if they are needed (POST+File/Body)
- When the whole request is parsed, the result is given to the ```handleRequest``` method of the ```Handler``` and is ready to be responded to
- In the ```handleRequest``` method, to the ```Request``` is attached a ```Response``` object (see below) that will serve the response data back to the client
- When the ```Response``` is sent, the client is closed and freed from the memory

### Rewrites and how do they work
- The ```Rewrites``` are used to rewrite the request url and/or inject get parameters for a specific request url path.
- All ```Rewrites``` are evaluated on the request in the order they have been added to the server.
- The ```Rewrite``` will change the request url only if the request url (excluding get parameters) is fully match
  the rewrite url, and when the optional ```Filter``` callback return true.
- Setting a ```Filter``` to the ```Rewrite``` enables to control when to apply the rewrite, decision can be based on
  request url, http version, request host/port/target host, get parameters or the request client's localIP or remoteIP.
- Two filter callbacks are provided: ```ON_AP_FILTER``` to execute the rewrite when request is made to the AP interface,
  ```ON_SAT_FILTER``` to execute the rewrite when request is made to the STA interface.
- The ```Rewrite``` can specify a target url with optional get parameters, e.g. ```/to-url?with=params```

### Handlers and how do they work
- The ```Handlers``` are used for executing specific actions to particular requests
- One ```Handler``` instance can be attached to any request and lives together with the server
- Setting a ```Filter``` to the ```Handler``` enables to control when to apply the handler, decision can be based on
  request url, http version, request host/port/target host, get parameters or the request client's localIP or remoteIP.
- Two filter callbacks are provided: ```ON_AP_FILTER``` to execute the rewrite when request is made to the AP interface,
  ```ON_SAT_FILTER``` to execute the rewrite when request is made to the STA interface.
- The ```canHandle``` method is used for handler specific control on whether the requests can be handled
  and for declaring any interesting headers that the ```Request``` should parse. Decision can be based on request
  method, request url, http version, request host/port/target host and get parameters
- Once a ```Handler``` is attached to given ```Request``` (```canHandle``` returned true)
  that ```Handler``` takes care to receive any file/data upload and attach a ```Response```
  once the ```Request``` has been fully parsed
- ```Handlers``` are evaluated in the order they are attached to the server. The ```canHandle``` is called only
  if the ```Filter``` that was set to the ```Handler``` return true.
- The first ```Handler``` that can handle the request is selected, not further ```Filter``` and ```canHandle``` are called.

### Responses and how do they work
- The ```Response``` objects are used to send the response data back to the client
- The ```Response``` object lives with the ```Request``` and is freed on end or disconnect
- Different techniques are used depending on the response type to send the data in packets
  returning back almost immediately and sending the next packet when this one is received.
  Any time in between is spent to run the user loop and handle other network packets
- Responding asynchronously is probably the most difficult thing for most to understand
- Many different options exist for the user to make responding a background task

## Libraries and projects that use AsyncWebServer
- [WebSocketToSerial](https://github.com/hallard/WebSocketToSerial)
- [Sattrack](https://github.com/Hopperpop/Sattrack)
- [ESP Radio](https://github.com/Edzelf/Esp-radio)

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

//List all collected headers (Compatibility)
int headers = request->headers();
int i;
for(i=0;i<headers;i++){
  Serial.printf("HEADER[%s]: %s\n", request->headerName(i).c_str(), request->header(i).c_str());
}

//get specific header by name (Compatibility)
if(request->hasHeader("MyHeader")){
  Serial.printf("MyHeader: %s\n", request->header("MyHeader").c_str());
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
if(request->hasParam("download"))
  AsyncWebParameter* p = request->getParam("download");

//Check if POST (but not File) parameter exists
if(request->hasParam("download", true))
  AsyncWebParameter* p = request->getParam("download", true);

//Check if FILE was uploaded
if(request->hasParam("download", true, true))
  AsyncWebParameter* p = request->getParam("download", true, true);

//List all parameters (Compatibility)
int args = request->args();
for(int i=0;i<args;i++){
  Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
}

//Check if parameter exists (Compatibility)
if(request->hasArg("download"))
  String arg = request->arg("download");
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
### Redirect to another URL
```cpp
//to local url
request->redirect("/login");

//to external url
request->redirect("http://esp8266.com");
```

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

## Serving static files
In addition to serving files from SPIFFS as described above, the server provide a dedicated handler that optimize the
performance of serving files from SPIFFS - ```AsyncStaticWebHandler```. Use ```server.serveStatic()``` function to
initialize and add a new instance of ```AsyncStaticWebHandler``` to the server.
The Handler will not handle the request if the file does not exists, e.g. the server will continue to look for another
handler that can handle the request.
Notice that you can chain setter functions to setup the handler, or keep a pointer to change it at a later time.

### Serving specific file by name
```cpp
// Serve the file "/www/page.htm" when request url is "/page.htm"
server.serveStatic("/page.htm", SPIFFS, "/www/page.htm");
```

### Serving files in directory
To serve files in a directory, the path to the files should specify a directory in SPIFFS and ends with "/".
```cpp
// Serve files in directory "/www/" when request url starts with "/"
// Request to the root or none existing files will try to server the defualt
// file name "index.htm" if exists
server.serveStatic("/", SPIFFS, "/www/");

// Server with different default file
server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("default.html");
```

### Specifying Cache-Control header
It is possible to specify Cache-Control header value to reduce the number of calls to the server once the client loaded
the files. For more information on Cache-Control values see [Cache-Control](https://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.9)
```cpp
// Cache responses for 10 minutes (600 seconds)
server.serveStatic("/", SPIFFS, "/www/").setCacheControl("max-age:600");

//*** Change Cache-Control after server setup ***

// During setup - keep a pointer to the handler
AsyncStaticWebHandler* handler = &server.serveStatic("/", SPIFFS, "/www/").setCacheControl("max-age:600");

// At a later event - change Cache-Control
handler->setCacheControl("max-age:30");
```

### Specifying Date-Modified header
It is possible to specify Date-Modified header to enable the server to return Not-Modified (304) response for requests
with "If-Modified-Since" header with the same value, instead of responding with the actual file content.
```cpp
// Update the date modified string every time files are updated
server.serveStatic("/", SPIFFS, "/www/").setLastModified("Mon, 20 Jun 2016 14:00:00 GMT");

//*** Chage last modified value at a later stage ***

// During setup - read last modified value from config or EEPROM
String date_modified = loadDateModified();
AsyncStaticWebHandler* handler = &server.serveStatic("/", SPIFFS, "/www/");
handler->setLastModified(date_modified);

// At a later event when files are updated
String date_modified = getNewDateModfied();
saveDateModified(date_modified); // Save for next reset
handler->setLastModified(date_modified);
```

## Using filters
Filters can be set to `Rewrite` or `Handler` in order to control when to apply the rewrite and consider the handler.
A filter is a callback function that evaluates the request and return a boolean `true` to include the item
or `false` to exclude it.
Two filter callback are provided for convince:
* `ON_STA_FILTER` - return true when requests are made to the STA (station mode) interface.
* `ON_AP_FILTER` - return true when requests are made to the AP (access point) interface.

### Serve different site files in AP mode
```cpp
server.serveStatic("/", SPIFFS, "/www/").setFilter(ON_STA_FILTER);
server.serveStatic("/", SPIFFS, "/ap/").setFilter(ON_AP_FILTER);
```

### Rewrite to different index on AP
```cpp
// Serve the file "/www/index-ap.htm" in AP, and the file "/www/index.htm" on STA
server.rewrite("/", "index.htm");
server.rewrite("/index.htm", "index-ap.htm").setFilter(ON_AP_FILTER);
server.serveStatic("/", SPIFFS, "/www/");
```

### Serving different hosts
```cpp
// Filter callback using request host
bool filterOnHost1(AsyncWebServerRequest *request) { return request->host() == "host1"; }

// Server setup: server files in "/host1/" to requests for "host1", and files in "/www/" otherwise.
server.serveStatic("/", SPIFFS, "/host1/").setFilter(filterOnHost1);
server.serveStatic("/", SPIFFS, "/www/");
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
ws.printf((uint32_t)client_id, arguments...);
//printf to all clients
ws.printfAll(arguments...);
//printf_P to a client
ws.printf_P((uint32_t)client_id, PSTR(format), arguments...);
//printfAll_P to all clients
ws.printf_P(PSTR(format), arguments...);
//send text to a client
ws.text((uint32_t)client_id, (char*)text);
ws.text((uint32_t)client_id, (uint8_t*)text, (size_t)len);
//send text from PROGMEM to a client
ws.text((uint32_t)client_id, PSTR("text"));
const char flash_text[] PROGMEM = "Text to send"
ws.text((uint32_t)client_id, FPSTR(flash_text));
//send text to all clients
ws.textAll((char*)text);
ws.textAll((uint8_t*)text, (size_t)len);
//send binary to a client
ws.binary((uint32_t)client_id, (char*)binary);
ws.binary((uint32_t)client_id, (uint8_t*)binary, (size_t)len);
//send binary from PROGMEM to a client
const uint8_t flash_binary[] PROGMEM = { 0x01, 0x02, 0x03, 0x04 };
ws.binary((uint32_t)client_id, flash_binary, 4);
//send binary to all clients
ws.binaryAll((char*)binary);
ws.binaryAll((uint8_t*)binary, (size_t)len);

//client methods
AsyncWebSocketClient * client;
//printf
client->printf(arguments...);
//printf_P
client->printf_P(PSTR(format), arguments...);
//send text
client->text((char*)text);
client->text((uint8_t*)text, (size_t)len);
//send text from PROGMEM
client->text(PSTR("text"));
const char flash_text[] PROGMEM = "Text to send";
client->text(FPSTR(flash_text));
//send binary
client->binary((char*)binary);
client->binary((uint8_t*)binary, (size_t)len);
//send binary from PROGMEM
const uint8_t flash_binary[] PROGMEM = { 0x01, 0x02, 0x03, 0x04 };
client->binary(flash_binary, 4);
```

## Async Event Source Plugin
The server includes EventSource (Server-Sent Events) plugin which can be used to send short text events to the browser.
Difference between EventSource and WebSockets is that EventSource is single direction, text-only protocol.

### Setup Event Source on the server
```cpp
AsyncWebServer server(80);
AsyncEventSource events("/events");

void setup(){
  // setup ......
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it gat is: %u\n", client->lastId());
    }
    //send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);
  // setup ......
}

void loop(){
  if(eventTriggered){ // your logic here
    //send event "myevent"
    client->send("my event content","myevent",millis());
  }
}
```

### Setup Event Source in the browser
```javascript
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

  source.addEventListener('myevent', function(e) {
    console.log("myevent", e.data);
  }, false);
}
```

## Setting up the server
```cpp
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)

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

  // attach AsyncEventSource
  server.addHandler(&events);

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

void loop(){
  static char temp[128];
  sprintf(temp, "Seconds since boot: %u", millis()/1000);
  events.send(temp, "time"); //send event "time"
}
```

### Methods for controlling websocket connections

```arduino
  // Disable client connections if it was activated
  if ( ws.enabled() )
    ws.enable(false);

  // enable client connections if it was disabled
  if ( !ws.enabled() )
    ws.enable(true);
```

Example of OTA code

```arduino
  // OTA callbacks
  ArduinoOTA.onStart([]() {
    // Clean SPIFFS
    SPIFFS.end();

    // Disable client connections    
    ws.enable(false);

    // Advertise connected clients what's going on
    ws.textAll("OTA Update Started");

    // Close them
    ws.closeAll();

  });

```
