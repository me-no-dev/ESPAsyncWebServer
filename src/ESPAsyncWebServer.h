#ifndef _ESPAsyncWebServer_H_
#define _ESPAsyncWebServer_H_

#include "Arduino.h"

#include <functional>
#include <ESPAsyncTCP.h>
#include "FS.h"

#include "StringArray.h"

class AsyncWebServer;
class AsyncWebServerRequest;
class AsyncWebServerResponse;
class AsyncWebHeader;
class AsyncWebParameter;
class AsyncWebHandler;

typedef enum {
  HTTP_ANY, HTTP_GET, HTTP_POST, HTTP_DELETE, HTTP_PUT, HTTP_PATCH, HTTP_HEAD, HTTP_OPTIONS
} WebRequestMethod;

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
      if(!data || !data.length() || data.indexOf(':') < 0)
        return;
      _name = data.substring(0, data.indexOf(':'));
      _value = data.substring(data.indexOf(':') + 2);
    }
    ~AsyncWebHeader(){}
    String name(){ return _name; }
    String value(){ return _value; }
    String toString(){ return String(_name+": "+_value+"\r\n"); }
};

/*
 * REQUEST :: Each incoming Client is wrapped inside a Request and both live together until disconnect
 * */

typedef std::function<size_t(uint8_t*, size_t)> AwsResponseFiller;

class AsyncWebServerRequest {
  private:
    AsyncClient* _client;
    AsyncWebServer* _server;
    AsyncWebHandler* _handler;
    AsyncWebServerResponse* _response;
    StringArray* _interestingHeaders;

    String _temp;
    uint8_t _parseState;

    WebRequestMethod _method;
    String _url;
    String _host;
    String _contentType;
    String _boundary;
    String _authorization;
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
    String _urlDecode(const String& text);

    bool _parseReqHead();
    bool _parseReqHeader();
    void _parseLine();
    void _parseByte(uint8_t data);
    void _parsePlainPostChar(uint8_t data);
    void _parseMultipartPostByte(uint8_t data, bool last);
    void _addGetParam(String param);

    void _handleUploadStart();
    void _handleUploadByte(uint8_t data, bool last);
    void _handleUploadEnd();

  public:
    File _tempFile;

    AsyncWebServerRequest(AsyncWebServer*, AsyncClient*);
    ~AsyncWebServerRequest();

    AsyncClient* client(){ return _client; }
    WebRequestMethod method(){ return _method; }
    String url(){ return _url; }
    String host(){ return _host; }
    String contentType(){ return _contentType; }
    size_t contentLength(){ return _contentLength; }
    bool multipart(){ return _isMultipart; }

    bool authenticate(const char * username, const char * password);
    bool authenticate(const char * hash);
    void requestAuthentication();

    void setHandler(AsyncWebHandler *handler){ _handler = handler; }
    void addInterestingHeader(String name);

    void send(AsyncWebServerResponse *response);
    void send(int code, String contentType=String(), String content=String());
    void send(FS &fs, String path, String contentType=String(), bool download=false);
    void send(Stream &stream, String contentType, size_t len);
    void send(String contentType, size_t len, AwsResponseFiller callback);

    AsyncWebServerResponse *beginResponse(int code, String contentType=String(), String content=String());
    AsyncWebServerResponse *beginResponse(FS &fs, String path, String contentType=String(), bool download=false);
    AsyncWebServerResponse *beginResponse(Stream &stream, String contentType, size_t len);
    AsyncWebServerResponse *beginResponse(String contentType, size_t len, AwsResponseFiller callback);

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

};

/*
 * HANDLER :: One instance can be attached to any Request (done by the Server)
 * */

class AsyncWebHandler {
  public:
    AsyncWebHandler* next;
    AsyncWebHandler(): next(NULL){}
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
    size_t _headLength;
    size_t _sentLength;
    size_t _ackedLength;
    WebResponseState _state;
    const char* _responseCodeToString(int code);

  public:
    AsyncWebServerResponse();
    virtual ~AsyncWebServerResponse();
    virtual void addHeader(String name, String value);
    virtual String _assembleHead();
    virtual bool _finished();
    virtual bool _failed();
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
  private:
    AsyncServer _server;
    AsyncWebHandler* _handlers;
    AsyncWebHandler* _catchAllHandler;
  public:
    AsyncWebServer(uint16_t port);
    ~AsyncWebServer(){}

    void begin();
    void addHandler(AsyncWebHandler* handler);

    void on(const char* uri, ArRequestHandlerFunction onRequest);
    void on(const char* uri, WebRequestMethod method, ArRequestHandlerFunction onRequest);
    void on(const char* uri, WebRequestMethod method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload);
    void on(const char* uri, WebRequestMethod method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody);

    void serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_header = NULL);

    void onNotFound(ArRequestHandlerFunction fn);  //called when handler is not assigned
    void onFileUpload(ArUploadHandlerFunction fn); //handle file uploads
    void onRequestBody(ArBodyHandlerFunction fn); //handle posts with plain body content (JSON often transmitted this way as a request)

    void _handleDisconnect(AsyncWebServerRequest *request);
    void _handleRequest(AsyncWebServerRequest *request);
};


#endif /* _AsyncWebServer_H_ */
