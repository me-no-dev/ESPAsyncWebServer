# ESPAsyncWebServer

[![Join the chat at https://gitter.im/me-no-dev/ESPAsyncWebServer](https://badges.gitter.im/me-no-dev/ESPAsyncWebServer.svg)](https://gitter.im/me-no-dev/ESPAsyncWebServer?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
Async Web Server for ESP8266 Arduino
Requires [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) to work

To use this library you need to have the latest git versions of either ESP8266 or ESP31B Arduino Core

## Why should you care
- Using asynchronous network means that you can handle more than one connection at the same time
- You are called once the request is ready and parsed
- When you send the response, you are immediately ready to handle other connections
  while the server is taking care of sending the response in the background
- Speed is OMG
- Easy to use API, HTTP Basic Authentication, ChunkedResponse
- Easily extendible to handle any type of content

## Important things to remember
- This is fully asynchronous server and as such does not run on the loop thread.
- You can not use yield or delay or any function that uses them inside the callbacks
- The server is smart enough to know when to close the connection and free resources
- You can not send more than one response to a single request

## Principles of operation

### The Async Web server
- Listens for connections
- Wraps the new clients into Request
- Keeps track of clients and cleans memory
- Manages Handlers and attaches them to Requests

### Request Life Cycle
- TCP connection is received by the server
- The connection is wrapped inside AsyncRequest object
- When the request head is received (type, url, get params, http version and host),
  the server goes through all attached Handlers(in the order they are attached) trying to find one
  that canHandle the given request. If none are found, the default(catch-all) handler is attached.
- The rest of the request is received, calling the handleUpload or handleBody methods of the Handler if they are needed (POST+File/Body)
- When the whole request is parsed, the result is given to the handleRequest method of the Handler and is ready to be responded to
- In the handleRequest method, to the Request is attached a Response object (see below) that will serve the response data back to the client
- When the response is sent, the client is closed and freed from the memory together with the Response

### Handlers and how do they work
- The Handlers are used for executing specific actions to particular requests
- One Handler instance can be attached to any request and lives together with the server
- The canHandle method is used for filtering the requests that can be handled
  and declaring any interesting headers that the Request should collect 
- Decision can be based on request method, request url, http version, request host/port/target host and GET parameters
- Once a Handler is attached to given Request (canHandle returned true)
  that handler takes care to receive any file/data upload and attach a Response
  once the request have been fully parsed
- Handler's canHandle is called in the order they are attached to the server.
  If a handler attached earlier returns true on canHandle, then this Hander's canHandle will never be called

### Responses and how do they work
- The Response objects are used to send the response data back to the client
- The Response object lives with the Request and is freed on end or disconnect
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
bool exists = hasParam("download");
AsyncWebParameter* p = request->getParam("download");

//Check if POST (but not File) parameter exists
bool exists = hasParam("download", true);
AsyncWebParameter* p = request->getParam("download", true);

//Check if FILE was uploaded
bool exists = hasParam("download", true, true);
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

## Basic Responses

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
request->send("text/plain", 128, [](uint8_t buffer, size_t maxLen) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //You will not be asked for more bytes once the content length has been reached.
  //Keep in mind that you can not delay or yield waiting for more data!
  //Send what you currently have and you will be asked for more again
  return mySource.read(buffer, maxLen);
});
```

### Respond with content using a callback and extra headers
```cpp
//send 128 bytes as plain text
AsyncWebServerResponse *response = request->beginResponse("text/plain", 128, [](uint8_t buffer, size_t maxLen) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //You will not be asked for more bytes once the content length has been reached.
  //Keep in mind that you can not delay or yield waiting for more data!
  //Send what you currently have and you will be asked for more again
  return mySource.read(buffer, maxLen);
});
response->addHeader("Server","ESP Async Web Server");
request->send(response);
```

## Advanced Responses

### Chunked Response
Used when content length is unknown. Works best if the client supports HTTP/1.1
```cpp
AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain", [](uint8_t buffer, size_t maxLen) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //You will be asked for more data until 0 is returned
  //Keep in mind that you can not delay or yield waiting for more data!
  return mySource.read(buffer, maxLen);
});
request->send(response);
```

### Print to response
```cpp
AsyncResponseStream *response = request->beginResponseStream("text/plain", 12);
response->print("Hello World!");
request->send(response);
```

### ArduinoJson Response
```cpp
#include "AsyncJson.h"
#include "ArduinoJson.h"


AsyncJsonResponse * response = new AsyncJsonResponse();
JsonObject& root = response->getRoot();
root["heap"] = ESP.getFreeHeap();
root["ssid"] = WiFi.SSID();

response->setLength();
request->send(response);
```




