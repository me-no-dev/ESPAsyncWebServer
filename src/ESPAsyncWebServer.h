/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _ESPAsyncWebServer_H_
#define _ESPAsyncWebServer_H_

#include "Arduino.h"

#include <functional>
#include <ESPAsyncTCP.h>
#include "FS.h"

#include "StringArray.h"

#if defined(ESP31B)
#include <ESP31BWiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error Platform not supported
#endif

#define DEBUGF(...) //Serial.printf(__VA_ARGS__)


class AsyncWebServer;
class AsyncWebServerRequest;
class AsyncWebServerResponse;
class AsyncWebHeader;
class AsyncWebParameter;
class AsyncWebRewrite;
class AsyncWebHandler;
class AsyncStaticWebHandler;
class AsyncCallbackWebHandler;
class AsyncResponseStream;

typedef enum {
  HTTP_GET     = 0b00000001,
  HTTP_POST    = 0b00000010,
  HTTP_DELETE  = 0b00000100,
  HTTP_PUT     = 0b00001000,
  HTTP_PATCH   = 0b00010000,
  HTTP_HEAD    = 0b00100000,
  HTTP_OPTIONS = 0b01000000,
  HTTP_ANY     = 0b01111111,
} WebRequestMethod;
typedef uint8_t WebRequestMethodComposite;

/*
 * PARAMETER :: Chainable object to hold GET/POST and FILE parameters
 * */

class AsyncWebParameter {
  private:
    String _name;
    String _value;
    size_t _size;
    bool _isForm;
    bool _isFile;

  public:
    AsyncWebParameter *next;

    AsyncWebParameter(String name, String value, bool form=false, bool file=false, size_t size=0): _name(name), _value(value), _size(size), _isForm(form), _isFile(file), next(NULL){}
    String name(){ return _name; }
    String value(){ return _value; }
    size_t size(){ return _size; }
    bool isPost(){ return _isForm; }
    bool isFile(){ return _isFile; }
};

/*
 * HEADER :: Chainable object to hold the headers
 * */

class AsyncWebHeader {
  private:
    String _name;
    String _value;

  public:
    AsyncWebHeader *next;

    AsyncWebHeader(String name, String value): _name(name), _value(value), next(NULL){}
    AsyncWebHeader(String data): _name(), _value(), next(NULL){
      if(!data) return;
      int index = data.indexOf(':');
      if (index < 0) return;
      _name = data.substring(0, index);
      _value = data.substring(index + 2);
    }
    ~AsyncWebHeader(){}
    String name(){ return _name; }
    String value(){ return _value; }
    String toString(){ return String(_name+": "+_value+"\r\n"); }
};

/*
 * REQUEST :: Each incoming Client is wrapped inside a Request and both live together until disconnect
 * */

typedef std::function<size_t(uint8_t*, size_t, size_t)> AwsResponseFiller;

class AsyncWebServerRequest {
  friend class AsyncWebServer;
  private:
    AsyncClient* _client;
    AsyncWebServer* _server;
    AsyncWebHandler* _handler;
    AsyncWebServerResponse* _response;
    StringArray* _interestingHeaders;

    String _temp;
    uint8_t _parseState;

    uint8_t _version;
    WebRequestMethodComposite _method;
    String _url;
    String _host;
    String _contentType;
    String _boundary;
    String _authorization;
    bool _isDigest;
    bool _isMultipart;
    bool _isPlainPost;
    bool _expectingContinue;
    size_t _contentLength;
    size_t _parsedLength;

    AsyncWebHeader *_headers;
    AsyncWebParameter *_params;

    uint8_t _multiParseState;
    uint8_t _boundaryPosition;
    size_t _itemStartIndex;
    size_t _itemSize;
    String _itemName;
    String _itemFilename;
    String _itemType;
    String _itemValue;
    uint8_t *_itemBuffer;
    size_t _itemBufferIndex;
    bool _itemIsFile;

    void _onPoll();
    void _onAck(size_t len, uint32_t time);
    void _onError(int8_t error);
    void _onTimeout(uint32_t time);
    void _onDisconnect();
    void _onData(void *buf, size_t len);

    void _addParam(AsyncWebParameter*);

    bool _parseReqHead();
    bool _parseReqHeader();
    void _parseLine();
    void _parsePlainPostChar(uint8_t data);
    void _parseMultipartPostByte(uint8_t data, bool last);
    void _addGetParams(String params);

    void _handleUploadStart();
    void _handleUploadByte(uint8_t data, bool last);
    void _handleUploadEnd();

  public:
    File _tempFile;
    void *_tempObject;
    AsyncWebServerRequest *next;

    AsyncWebServerRequest(AsyncWebServer*, AsyncClient*);
    ~AsyncWebServerRequest();

    AsyncClient* client(){ return _client; }
    uint8_t version(){ return _version; }
    WebRequestMethodComposite method(){ return _method; }
    String url(){ return _url; }
    String host(){ return _host; }
    String contentType(){ return _contentType; }
    size_t contentLength(){ return _contentLength; }
    bool multipart(){ return _isMultipart; }
    const char * methodToString();


    //hash is the string representation of:
    // base64(user:pass) for basic or
    // user:realm:md5(user:realm:pass) for digest
    bool authenticate(const char * hash);
    bool authenticate(const char * username, const char * password, const char * realm = NULL, bool passwordIsHash = false);
    void requestAuthentication(const char * realm = NULL, bool isDigest = true);

    void setHandler(AsyncWebHandler *handler){ _handler = handler; }
    void addInterestingHeader(String name);

    void redirect(String url);

    void send(AsyncWebServerResponse *response);
    void send(int code, String contentType=String(), String content=String());
    void send(FS &fs, String path, String contentType=String(), bool download=false);
    void send(File content, String path, String contentType=String(), bool download=false);
    void send(Stream &stream, String contentType, size_t len);
    void send(String contentType, size_t len, AwsResponseFiller callback);
    void sendChunked(String contentType, AwsResponseFiller callback);
    void send_P(int code, String contentType, const uint8_t * content, size_t len);
    void send_P(int code, String contentType, PGM_P content);

    AsyncWebServerResponse *beginResponse(int code, String contentType=String(), String content=String());
    AsyncWebServerResponse *beginResponse(FS &fs, String path, String contentType=String(), bool download=false);
    AsyncWebServerResponse *beginResponse(File content, String path, String contentType=String(), bool download=false);
    AsyncWebServerResponse *beginResponse(Stream &stream, String contentType, size_t len);
    AsyncWebServerResponse *beginResponse(String contentType, size_t len, AwsResponseFiller callback);
    AsyncWebServerResponse *beginChunkedResponse(String contentType, AwsResponseFiller callback);
    AsyncResponseStream *beginResponseStream(String contentType, size_t bufferSize=1460);
    AsyncWebServerResponse *beginResponse_P(int code, String contentType, const uint8_t * content, size_t len);
    AsyncWebServerResponse *beginResponse_P(int code, String contentType, PGM_P content);

    int headers();                     // get header count
    bool hasHeader(String name);
    AsyncWebHeader* getHeader(String name);
    AsyncWebHeader* getHeader(int num);

    int params();                      // get arguments count
    bool hasParam(String name, bool post=false, bool file=false);
    AsyncWebParameter* getParam(String name, bool post=false, bool file=false);
    AsyncWebParameter* getParam(int num);

    int args(){ return params(); }  // get arguments count
    String arg(const char* name);   // get request argument value by name
    String arg(int i);              // get request argument value by number
    String argName(int i);          // get request argument name by number
    bool hasArg(const char* name);  // check if argument exists

    String header(const char* name);   // get request header value by name
    String header(int i);              // get request header value by number
    String headerName(int i);          // get request header name by number
    bool hasHeader(const char* name);  // check if header exists

    String urlDecode(const String& text);
};

/*
 * FILTER :: Callback to filter AsyncWebRewrite and AsyncWebHandler (done by the Server)
 * */

typedef std::function<bool(AsyncWebServerRequest *request)> ArRequestFilterFunction;

static bool ON_STA_FILTER(AsyncWebServerRequest *request) {
  return WiFi.localIP() == request->client()->localIP();
}

static bool ON_AP_FILTER(AsyncWebServerRequest *request) {
  return WiFi.localIP() != request->client()->localIP();
}

/*
 * REWRITE :: One instance can be handle any Request (done by the Server)
 * */

class AsyncWebRewrite {
  protected:
    String _from;
    String _toUrl;
    String _params;
    ArRequestFilterFunction _filter;
  public:
    AsyncWebRewrite* next;
    AsyncWebRewrite(const char* from, const char* to): _from(from), _toUrl(to), _params(String()), _filter(NULL), next(NULL){
      int index = _toUrl.indexOf('?');
      if (index > 0) {
        _params = _toUrl.substring(index +1);
        _toUrl = _toUrl.substring(0, index);
      }
    }
    AsyncWebRewrite& setFilter(ArRequestFilterFunction fn) { _filter = fn; return *this; }
    bool filter(AsyncWebServerRequest *request){ return _filter == NULL || _filter(request); }
    String from(void) { return _from; }
    String toUrl(void) { return _toUrl; }
    String params(void) { return _params; }
};

/*
 * HANDLER :: One instance can be attached to any Request (done by the Server)
 * */

class AsyncWebHandler {
  protected:
    ArRequestFilterFunction _filter;
  public:
    AsyncWebHandler* next;
    AsyncWebHandler(): next(NULL){}
    AsyncWebHandler& setFilter(ArRequestFilterFunction fn) { _filter = fn; return *this; }
    bool filter(AsyncWebServerRequest *request){ return _filter == NULL || _filter(request); }
    virtual ~AsyncWebHandler(){}
    virtual bool canHandle(AsyncWebServerRequest *request){ return false; }
    virtual void handleRequest(AsyncWebServerRequest *request){}
    virtual void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){}
    virtual void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){}
};

/*
 * RESPONSE :: One instance is created for each Request (attached by the Handler)
 * */

typedef enum {
  RESPONSE_SETUP, RESPONSE_HEADERS, RESPONSE_CONTENT, RESPONSE_WAIT_ACK, RESPONSE_END, RESPONSE_FAILED
} WebResponseState;

class AsyncWebServerResponse {
  protected:
    int _code;
    AsyncWebHeader *_headers;
    String _contentType;
    size_t _contentLength;
    bool _sendContentLength;
    bool _chunked;
    size_t _headLength;
    size_t _sentLength;
    size_t _ackedLength;
    size_t _writtenLength;
    WebResponseState _state;
    const char* _responseCodeToString(int code);

  public:
    AsyncWebServerResponse();
    virtual ~AsyncWebServerResponse();
    virtual void setCode(int code);
    virtual void setContentLength(size_t len);
    virtual void setContentType(String type);
    virtual void addHeader(String name, String value);
    virtual String _assembleHead(uint8_t version);
    virtual bool _started();
    virtual bool _finished();
    virtual bool _failed();
    virtual bool _sourceValid();
    virtual void _respond(AsyncWebServerRequest *request);
    virtual size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
};

/*
 * SERVER :: One instance
 * */

typedef std::function<void(AsyncWebServerRequest *request)> ArRequestHandlerFunction;
typedef std::function<void(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)> ArUploadHandlerFunction;
typedef std::function<void(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)> ArBodyHandlerFunction;

class AsyncWebServer {
  protected:
    AsyncServer _server;
    AsyncWebRewrite* _rewrites;
    AsyncWebHandler* _handlers;
    AsyncWebHandler* _catchAllHandler;

  public:
    AsyncWebServer(uint16_t port);
    ~AsyncWebServer();

    void begin();

#if ASYNC_TCP_SSL_ENABLED
    void onSslFileRequest(AcSSlFileHandler cb, void* arg);
    void beginSecure(const char *cert, const char *private_key_file, const char *password);
#endif

    AsyncWebRewrite& addRewrite(AsyncWebRewrite* rewrite);

    AsyncWebRewrite& rewrite(const char* from, const char* to);

    AsyncWebHandler& addHandler(AsyncWebHandler* handler);

    AsyncCallbackWebHandler& on(const char* uri, ArRequestHandlerFunction onRequest);
    AsyncCallbackWebHandler& on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest);
    AsyncCallbackWebHandler& on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload);
    AsyncCallbackWebHandler& on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody);

    AsyncStaticWebHandler& serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control = NULL);

    void onNotFound(ArRequestHandlerFunction fn);  //called when handler is not assigned
    void onFileUpload(ArUploadHandlerFunction fn); //handle file uploads
    void onRequestBody(ArBodyHandlerFunction fn); //handle posts with plain body content (JSON often transmitted this way as a request)

    void _handleDisconnect(AsyncWebServerRequest *request);
    void _attachHandler(AsyncWebServerRequest *request);
    void _rewriteRequest(AsyncWebServerRequest *request);
};

#include "WebResponseImpl.h"
#include "WebHandlerImpl.h"
#include "AsyncWebSocket.h"
#include "AsyncEventSource.h"

#endif /* _AsyncWebServer_H_ */
