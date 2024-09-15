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

#include "FS.h"
#include <deque>
#include <functional>
#include <list>
#include <unordered_map>
#include <vector>

#ifdef ESP32
  #include <AsyncTCP.h>
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(TARGET_RP2040)
  #include <AsyncTCP_RP2040W.h>
  #include <HTTP_Method.h>
  #include <WiFi.h>
  #include <http_parser.h>
#else
  #error Platform not supported
#endif

#include "literals.h"

#define ASYNCWEBSERVER_VERSION          "3.3.1"
#define ASYNCWEBSERVER_VERSION_MAJOR    3
#define ASYNCWEBSERVER_VERSION_MINOR    3
#define ASYNCWEBSERVER_VERSION_REVISION 0
#define ASYNCWEBSERVER_FORK_mathieucarbou

#ifdef ASYNCWEBSERVER_REGEX
  #define ASYNCWEBSERVER_REGEX_ATTRIBUTE
#else
  #define ASYNCWEBSERVER_REGEX_ATTRIBUTE __attribute__((warning("ASYNCWEBSERVER_REGEX not defined")))
#endif

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
class AsyncMiddlewareChain;

#if defined(TARGET_RP2040)
typedef enum http_method WebRequestMethod;
#else
  #ifndef WEBSERVER_H
typedef enum {
  HTTP_GET = 0b00000001,
  HTTP_POST = 0b00000010,
  HTTP_DELETE = 0b00000100,
  HTTP_PUT = 0b00001000,
  HTTP_PATCH = 0b00010000,
  HTTP_HEAD = 0b00100000,
  HTTP_OPTIONS = 0b01000000,
  HTTP_ANY = 0b01111111,
} WebRequestMethod;
  #endif
#endif

#ifndef HAVE_FS_FILE_OPEN_MODE
namespace fs {
  class FileOpenMode {
    public:
      static const char* read;
      static const char* write;
      static const char* append;
  };
};
#else
  #include "FileOpenMode.h"
#endif

// if this value is returned when asked for data, packet will not be sent and you will be asked for data again
#define RESPONSE_TRY_AGAIN          0xFFFFFFFF
#define RESPONSE_STREAM_BUFFER_SIZE 1460

typedef uint8_t WebRequestMethodComposite;
typedef std::function<void(void)> ArDisconnectHandler;

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
    AsyncWebParameter(const String& name, const String& value, bool form = false, bool file = false, size_t size = 0) : _name(name), _value(value), _size(size), _isForm(form), _isFile(file) {}
    const String& name() const { return _name; }
    const String& value() const { return _value; }
    size_t size() const { return _size; }
    bool isPost() const { return _isForm; }
    bool isFile() const { return _isFile; }
};

/*
 * HEADER :: Chainable object to hold the headers
 * */

class AsyncWebHeader {
  private:
    String _name;
    String _value;

  public:
    AsyncWebHeader() = default;
    AsyncWebHeader(const AsyncWebHeader&) = default;

    AsyncWebHeader(const char* name, const char* value) : _name(name), _value(value) {}
    AsyncWebHeader(const String& name, const String& value) : _name(name), _value(value) {}
    AsyncWebHeader(const String& data) {
      if (!data)
        return;
      int index = data.indexOf(':');
      if (index < 0)
        return;
      _name = data.substring(0, index);
      _value = data.substring(index + 2);
    }

    AsyncWebHeader& operator=(const AsyncWebHeader&) = default;

    const String& name() const { return _name; }
    const String& value() const { return _value; }
    String toString() const {
      String str;
      str.reserve(_name.length() + _value.length() + 2);
      str.concat(_name);
      str.concat((char)0x3a);
      str.concat((char)0x20);
      str.concat(_value);
      str.concat(asyncsrv::T_rn);
      return str;
    }
};

/*
 * REQUEST :: Each incoming Client is wrapped inside a Request and both live together until disconnect
 * */

typedef enum { RCT_NOT_USED = -1,
               RCT_DEFAULT = 0,
               RCT_HTTP,
               RCT_WS,
               RCT_EVENT,
               RCT_MAX } RequestedConnectionType;

typedef std::function<size_t(uint8_t*, size_t, size_t)> AwsResponseFiller;
typedef std::function<String(const String&)> AwsTemplateProcessor;

class AsyncWebServerRequest {
    using File = fs::File;
    using FS = fs::FS;
    friend class AsyncWebServer;
    friend class AsyncCallbackWebHandler;

  private:
    AsyncClient* _client;
    AsyncWebServer* _server;
    AsyncWebHandler* _handler;
    AsyncWebServerResponse* _response;
    ArDisconnectHandler _onDisconnectfn;

    // response is sent
    bool _sent = false;

    String _temp;
    uint8_t _parseState;

    uint8_t _version;
    WebRequestMethodComposite _method;
    String _url;
    String _host;
    String _contentType;
    String _boundary;
    String _authorization;
    RequestedConnectionType _reqconntype;
    bool _isDigest;
    bool _isMultipart;
    bool _isPlainPost;
    bool _expectingContinue;
    size_t _contentLength;
    size_t _parsedLength;

    std::list<AsyncWebHeader> _headers;
    std::list<AsyncWebParameter> _params;
    std::vector<String> _pathParams;

    std::unordered_map<const char*, String, std::hash<const char*>, std::equal_to<const char*>> _attributes;

    uint8_t _multiParseState;
    uint8_t _boundaryPosition;
    size_t _itemStartIndex;
    size_t _itemSize;
    String _itemName;
    String _itemFilename;
    String _itemType;
    String _itemValue;
    uint8_t* _itemBuffer;
    size_t _itemBufferIndex;
    bool _itemIsFile;

    void _onPoll();
    void _onAck(size_t len, uint32_t time);
    void _onError(int8_t error);
    void _onTimeout(uint32_t time);
    void _onDisconnect();
    void _onData(void* buf, size_t len);

    void _addPathParam(const char* param);

    bool _parseReqHead();
    bool _parseReqHeader();
    void _parseLine();
    void _parsePlainPostChar(uint8_t data);
    void _parseMultipartPostByte(uint8_t data, bool last);
    void _addGetParams(const String& params);

    void _handleUploadStart();
    void _handleUploadByte(uint8_t data, bool last);
    void _handleUploadEnd();

  public:
    File _tempFile;
    void* _tempObject;

    AsyncWebServerRequest(AsyncWebServer*, AsyncClient*);
    ~AsyncWebServerRequest();

    AsyncClient* client() { return _client; }
    uint8_t version() const { return _version; }
    WebRequestMethodComposite method() const { return _method; }
    const String& url() const { return _url; }
    const String& host() const { return _host; }
    const String& contentType() const { return _contentType; }
    size_t contentLength() const { return _contentLength; }
    bool multipart() const { return _isMultipart; }

#ifndef ESP8266
    const char* methodToString() const;
    const char* requestedConnTypeToString() const;
#else
    const __FlashStringHelper* methodToString() const;
    const __FlashStringHelper* requestedConnTypeToString() const;
#endif

    RequestedConnectionType requestedConnType() const { return _reqconntype; }
    bool isExpectedRequestedConnType(RequestedConnectionType erct1, RequestedConnectionType erct2 = RCT_NOT_USED, RequestedConnectionType erct3 = RCT_NOT_USED);
    void onDisconnect(ArDisconnectHandler fn);

    // hash is the string representation of:
    //  base64(user:pass) for basic or
    //  user:realm:md5(user:realm:pass) for digest
    bool authenticate(const char* hash);
    bool authenticate(const char* username, const char* password, const char* realm = NULL, bool passwordIsHash = false);
    void requestAuthentication(const char* realm = NULL, bool isDigest = true);

    void setHandler(AsyncWebHandler* handler) { _handler = handler; }

#ifndef ESP8266
    [[deprecated("All headers are now collected. Use removeHeader(name) or HeaderFreeMiddleware if you really need to free some headers.")]]
#endif
    void addInterestingHeader(__unused const char* name) {
    }
#ifndef ESP8266
    [[deprecated("All headers are now collected. Use removeHeader(name) or HeaderFreeMiddleware if you really need to free some headers.")]]
#endif
    void addInterestingHeader(__unused const String& name) {
    }

    /**
     * @brief issue 302 redirect response
     *
     * @param url
     */
    void redirect(const char* url);
    void redirect(const String& url) { return redirect(url.c_str()); };

    void send(AsyncWebServerResponse* response);
    AsyncWebServerResponse* getResponse() const { return _response; }

    void send(int code, const char* contentType = asyncsrv::empty, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr) { send(beginResponse(code, contentType, content, callback)); }
    void send(int code, const String& contentType, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr) { send(beginResponse(code, contentType.c_str(), content, callback)); }
    void send(int code, const String& contentType, const String& content, AwsTemplateProcessor callback = nullptr) { send(beginResponse(code, contentType.c_str(), content.c_str(), callback)); }

    void send(int code, const char* contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr) { send(beginResponse(code, contentType, content, len, callback)); }
    void send(int code, const String& contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr) { send(beginResponse(code, contentType, content, len, callback)); }

    void send(FS& fs, const String& path, const char* contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr) {
      if (fs.exists(path) || (!download && fs.exists(path + asyncsrv::T__gz))) {
        send(beginResponse(fs, path, contentType, download, callback));
      } else
        send(404);
    }
    void send(FS& fs, const String& path, const String& contentType, bool download = false, AwsTemplateProcessor callback = nullptr) { send(fs, path, contentType.c_str(), download, callback); }

    void send(File content, const String& path, const char* contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr) {
      if (content) {
        send(beginResponse(content, path, contentType, download, callback));
      } else
        send(404);
    }
    void send(File content, const String& path, const String& contentType, bool download = false, AwsTemplateProcessor callback = nullptr) { send(content, path, contentType.c_str(), download, callback); }

    void send(Stream& stream, const char* contentType, size_t len, AwsTemplateProcessor callback = nullptr) { send(beginResponse(stream, contentType, len, callback)); }
    void send(Stream& stream, const String& contentType, size_t len, AwsTemplateProcessor callback = nullptr) { send(beginResponse(stream, contentType, len, callback)); }

    void send(const char* contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) { send(beginResponse(contentType, len, callback, templateCallback)); }
    void send(const String& contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) { send(beginResponse(contentType, len, callback, templateCallback)); }

    void sendChunked(const char* contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) { send(beginChunkedResponse(contentType, callback, templateCallback)); }
    void sendChunked(const String& contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) { send(beginChunkedResponse(contentType, callback, templateCallback)); }

#ifndef ESP8266
    [[deprecated("Replaced by send(int code, const String& contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr)")]]
#endif
    void send_P(int code, const String& contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr) {
      send(code, contentType, content, len, callback);
    }
#ifndef ESP8266
    [[deprecated("Replaced by send(int code, const String& contentType, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr)")]]
    void send_P(int code, const String& contentType, PGM_P content, AwsTemplateProcessor callback = nullptr) {
      send(code, contentType, content, callback);
    }
#else
    void send_P(int code, const String& contentType, PGM_P content, AwsTemplateProcessor callback = nullptr) {
      send(beginResponse_P(code, contentType, content, callback));
    }
#endif

    AsyncWebServerResponse* beginResponse(int code, const char* contentType = asyncsrv::empty, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr);
    AsyncWebServerResponse* beginResponse(int code, const String& contentType, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr) { return beginResponse(code, contentType.c_str(), content, callback); }
    AsyncWebServerResponse* beginResponse(int code, const String& contentType, const String& content, AwsTemplateProcessor callback = nullptr) { return beginResponse(code, contentType.c_str(), content.c_str(), callback); }

    AsyncWebServerResponse* beginResponse(int code, const char* contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr);
    AsyncWebServerResponse* beginResponse(int code, const String& contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr) { return beginResponse(code, contentType.c_str(), content, len, callback); }

    AsyncWebServerResponse* beginResponse(FS& fs, const String& path, const char* contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr);
    AsyncWebServerResponse* beginResponse(FS& fs, const String& path, const String& contentType = emptyString, bool download = false, AwsTemplateProcessor callback = nullptr) { return beginResponse(fs, path, contentType.c_str(), download, callback); }

    AsyncWebServerResponse* beginResponse(File content, const String& path, const char* contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr);
    AsyncWebServerResponse* beginResponse(File content, const String& path, const String& contentType = emptyString, bool download = false, AwsTemplateProcessor callback = nullptr) { return beginResponse(content, path, contentType.c_str(), download, callback); }

    AsyncWebServerResponse* beginResponse(Stream& stream, const char* contentType, size_t len, AwsTemplateProcessor callback = nullptr);
    AsyncWebServerResponse* beginResponse(Stream& stream, const String& contentType, size_t len, AwsTemplateProcessor callback = nullptr) { return beginResponse(stream, contentType.c_str(), len, callback); }

    AsyncWebServerResponse* beginResponse(const char* contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr);
    AsyncWebServerResponse* beginResponse(const String& contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) { return beginResponse(contentType.c_str(), len, callback, templateCallback); }

    AsyncWebServerResponse* beginChunkedResponse(const char* contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr);
    AsyncWebServerResponse* beginChunkedResponse(const String& contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr);

    AsyncResponseStream* beginResponseStream(const char* contentType, size_t bufferSize = RESPONSE_STREAM_BUFFER_SIZE);
    AsyncResponseStream* beginResponseStream(const String& contentType, size_t bufferSize = RESPONSE_STREAM_BUFFER_SIZE) { return beginResponseStream(contentType.c_str(), bufferSize); }

#ifndef ESP8266
    [[deprecated("Replaced by beginResponse(int code, const String& contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr)")]]
#endif
    AsyncWebServerResponse* beginResponse_P(int code, const String& contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr) {
      return beginResponse(code, contentType.c_str(), content, len, callback);
    }
#ifndef ESP8266
    [[deprecated("Replaced by beginResponse(int code, const String& contentType, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr)")]]
#endif
    AsyncWebServerResponse* beginResponse_P(int code, const String& contentType, PGM_P content, AwsTemplateProcessor callback = nullptr);

    /**
     * @brief Get the Request parameter by name
     *
     * @param name
     * @param post
     * @param file
     * @return const AsyncWebParameter*
     */
    const AsyncWebParameter* getParam(const char* name, bool post = false, bool file = false) const;

    const AsyncWebParameter* getParam(const String& name, bool post = false, bool file = false) const { return getParam(name.c_str(), post, file); };
#ifdef ESP8266
    const AsyncWebParameter* getParam(const __FlashStringHelper* data, bool post, bool file) const;
#endif

    /**
     * @brief Get request parameter by number
     * i.e., n-th parameter
     * @param num
     * @return const AsyncWebParameter*
     */
    const AsyncWebParameter* getParam(size_t num) const;

    size_t args() const { return params(); } // get arguments count

    // get request argument value by name
    const String& arg(const char* name) const;
    // get request argument value by name
    const String& arg(const String& name) const { return arg(name.c_str()); };
#ifdef ESP8266
    const String& arg(const __FlashStringHelper* data) const; // get request argument value by F(name)
#endif
    const String& arg(size_t i) const;     // get request argument value by number
    const String& argName(size_t i) const; // get request argument name by number
    bool hasArg(const char* name) const;   // check if argument exists
    bool hasArg(const String& name) const { return hasArg(name.c_str()); };
#ifdef ESP8266
    bool hasArg(const __FlashStringHelper* data) const; // check if F(argument) exists
#endif

    const String& ASYNCWEBSERVER_REGEX_ATTRIBUTE pathArg(size_t i) const;

    // get request header value by name
    const String& header(const char* name) const;
    const String& header(const String& name) const { return header(name.c_str()); };

#ifdef ESP8266
    const String& header(const __FlashStringHelper* data) const; // get request header value by F(name)
#endif

    const String& header(size_t i) const;     // get request header value by number
    const String& headerName(size_t i) const; // get request header name by number

    size_t headers() const; // get header count

    // check if header exists
    bool hasHeader(const char* name) const;
    bool hasHeader(const String& name) const { return hasHeader(name.c_str()); };
#ifdef ESP8266
    bool hasHeader(const __FlashStringHelper* data) const; // check if header exists
#endif

    const AsyncWebHeader* getHeader(const char* name) const;
    const AsyncWebHeader* getHeader(const String& name) const { return getHeader(name.c_str()); };
#ifdef ESP8266
    const AsyncWebHeader* getHeader(const __FlashStringHelper* data) const;
#endif

    const AsyncWebHeader* getHeader(size_t num) const;

    const std::list<AsyncWebHeader>& getHeaders() const { return _headers; }

    size_t getHeaderNames(std::vector<const char*>& names) const {
      names.clear();
      const size_t size = _headers.size();
      names.reserve(size);
      for (const auto& h : _headers) {
        names.push_back(h.name().c_str());
      }
      return size;
    }

    // Remove a header from the request.
    // It will free the memory and prevent the header to be seen during request processing.
    bool removeHeader(const char* name) {
      const size_t size = _headers.size();
      _headers.remove_if([name](const AsyncWebHeader& header) { return header.name().equalsIgnoreCase(name); });
      return size != _headers.size();
    }
    // Remove all request headers.
    void removeHeaders() { _headers.clear(); }

    size_t params() const; // get arguments count
    bool hasParam(const char* name, bool post = false, bool file = false) const;
    bool hasParam(const String& name, bool post = false, bool file = false) const { return hasParam(name.c_str(), post, file); };
#ifdef ESP8266
    bool hasParam(const __FlashStringHelper* data, bool post = false, bool file = false) const { return hasParam(String(data).c_str(), post, file); };
#endif

    // REQUEST ATTRIBUTES

    void setAttribute(const char* name, const char* value) { _attributes[name] = value; }
    void setAttribute(const char* name, bool value) { _attributes[name] = value ? "1" : emptyString; }
    void setAttribute(const char* name, long value) { _attributes[name] = String(value); }
    void setAttribute(const char* name, float value, unsigned int decimalPlaces = 2) { _attributes[name] = String(value, decimalPlaces); }
    void setAttribute(const char* name, double value, unsigned int decimalPlaces = 2) { _attributes[name] = String(value, decimalPlaces); }

    bool hasAttribute(const char* name) const { return _attributes.find(name) != _attributes.end(); }

    const String& getAttribute(const char* name, const String& defaultValue = emptyString) const {
      auto it = _attributes.find(name);
      return it != _attributes.end() ? it->second : defaultValue;
    }
    bool getAttribute(const char* name, bool defaultValue) const {
      auto it = _attributes.find(name);
      return it != _attributes.end() ? it->second == "1" : defaultValue;
    }
    long getAttribute(const char* name, long defaultValue) const {
      auto it = _attributes.find(name);
      return it != _attributes.end() ? it->second.toInt() : defaultValue;
    }
    float getAttribute(const char* name, float defaultValue) const {
      auto it = _attributes.find(name);
      return it != _attributes.end() ? it->second.toFloat() : defaultValue;
    }
    double getAttribute(const char* name, double defaultValue) const {
      auto it = _attributes.find(name);
      return it != _attributes.end() ? it->second.toDouble() : defaultValue;
    }

    String urlDecode(const String& text) const;
};

/*
 * FILTER :: Callback to filter AsyncWebRewrite and AsyncWebHandler (done by the Server)
 * */

using ArRequestFilterFunction = std::function<bool(AsyncWebServerRequest* request)>;

bool ON_STA_FILTER(AsyncWebServerRequest* request);

bool ON_AP_FILTER(AsyncWebServerRequest* request);

/*
 * MIDDLEWARE :: Request interceptor, assigned to a AsyncWebHandler (or the server), which can be used:
 * 1. to run some code before the final handler is executed (e.g. check authentication)
 * 2. decide whether to proceed or not with the next handler
 * */

using ArMiddlewareNext = std::function<void(void)>;
using ArMiddlewareCallback = std::function<void(AsyncWebServerRequest* request, ArMiddlewareNext next)>;

// Middleware is a base class for all middleware
class AsyncMiddleware {
  public:
    virtual ~AsyncMiddleware() {}
    virtual void run(__unused AsyncWebServerRequest* request, __unused ArMiddlewareNext next) {
      return next();
    };

  private:
    friend class AsyncWebHandler;
    friend class AsyncEventSource;
    friend class AsyncMiddlewareChain;
    bool _freeOnRemoval = false;
};

// Create a custom middleware by providing an anonymous callback function
class AsyncMiddlewareFunction : public AsyncMiddleware {
  public:
    AsyncMiddlewareFunction(ArMiddlewareCallback fn) : _fn(fn) {}
    void run(AsyncWebServerRequest* request, ArMiddlewareNext next) override { return _fn(request, next); };

  private:
    ArMiddlewareCallback _fn;
};

// For internal use only: super class to add/remove middleware to server or handlers
class AsyncMiddlewareChain {
  public:
    virtual ~AsyncMiddlewareChain() {
      for (AsyncMiddleware* m : _middlewares)
        if (m->_freeOnRemoval)
          delete m;
    }
    void addMiddleware(ArMiddlewareCallback fn) {
      AsyncMiddlewareFunction* m = new AsyncMiddlewareFunction(fn);
      m->_freeOnRemoval = true;
      _middlewares.emplace_back(m);
    }
    void addMiddleware(AsyncMiddleware* middleware) { _middlewares.emplace_back(middleware); }
    void addMiddlewares(std::vector<AsyncMiddleware*> middlewares) {
      for (AsyncMiddleware* m : middlewares)
        addMiddleware(m);
    }
    bool removeMiddleware(AsyncMiddleware* middleware) {
      // remove all middlewares from _middlewares vector being equal to middleware, delete them having _freeOnRemoval flag to true and resize the vector.
      const size_t size = _middlewares.size();
      _middlewares.erase(std::remove_if(_middlewares.begin(), _middlewares.end(), [middleware](AsyncMiddleware* m) {
                           if (m == middleware) {
                             if (m->_freeOnRemoval)
                               delete m;
                             return true;
                           }
                           return false;
                         }),
                         _middlewares.end());
      return size != _middlewares.size();
    }
    // For internal use only
    void _runChain(AsyncWebServerRequest* request, ArMiddlewareNext finalizer) {
      if (!_middlewares.size())
        return finalizer();
      ArMiddlewareNext next;
      std::list<AsyncMiddleware*>::iterator it = _middlewares.begin();
      next = [this, &next, &it, request, finalizer]() {
        if (it == _middlewares.end())
          return finalizer();
        AsyncMiddleware* m = *it;
        it++;
        return m->run(request, next);
      };
      return next();
    }

  protected:
    std::list<AsyncMiddleware*> _middlewares;
};

// AuthenticationMiddleware is a middleware that checks if the request is authenticated
class AuthenticationMiddleware : public AsyncMiddleware {
  public:
    typedef enum {
      AUTH_NONE,
      AUTH_BASIC,
      AUTH_DIGEST
    } AuthType;

    void setUsername(const char* username) { _username = username; }
    void setPassword(const char* password) { _password = password; }
    void setRealm(const char* realm) { _realm = realm; }
    void setPasswordIsHash(bool passwordIsHash) { _hash = passwordIsHash; }
    void setAuthType(AuthType authType) { _authType = authType; }

    bool allowed(AsyncWebServerRequest* request) {
      return _authType == AUTH_NONE || !_username.length() || !_password.length() || request->authenticate(_username.c_str(), _password.c_str(), _realm, _hash);
    }

    void run(AsyncWebServerRequest* request, ArMiddlewareNext next) {
      return allowed(request) ? next() : request->requestAuthentication(_realm, _authType == AUTH_DIGEST);
    }

  private:
    String _username;
    String _password;
    const char* _realm = nullptr;
    bool _hash = false;
    AuthType _authType = AUTH_DIGEST;
};

using ArAuthorizeFunction = std::function<bool(AsyncWebServerRequest* request)>;
// AuthorizationMiddleware is a middleware that checks if the request is authorized
class AuthorizationMiddleware : public AsyncMiddleware {
  public:
    AuthorizationMiddleware(ArAuthorizeFunction authorizeConnectHandler) : _code(403), _authz(authorizeConnectHandler) {}
    AuthorizationMiddleware(int code, ArAuthorizeFunction authorizeConnectHandler) : _code(code), _authz(authorizeConnectHandler) {}
    void run(AsyncWebServerRequest* request, ArMiddlewareNext next) {
      if (_authz && !_authz(request))
        return request->send(_code);
      return next();
    }

  private:
    int _code;
    ArAuthorizeFunction _authz;
};

// remove all headers from the incoming request except the ones provided in the constructor
class HeaderFreeMiddleware : public AsyncMiddleware {
  public:
    void keep(const char* name) { _toKeep.push_back(name); }
    void unKeep(const char* name) { _toKeep.erase(std::remove(_toKeep.begin(), _toKeep.end(), name), _toKeep.end()); }
    void run(AsyncWebServerRequest* request, ArMiddlewareNext next) {
      std::vector<const char*> reqHeaders;
      request->getHeaderNames(reqHeaders);
      for (const char* h : reqHeaders) {
        bool keep = false;
        for (const char* k : _toKeep) {
          if (strcasecmp(h, k) == 0) {
            keep = true;
            break;
          }
        }
        if (!keep) {
          request->removeHeader(h);
        }
      }
      next();
    }

  private:
    std::vector<const char*> _toKeep;
};

// filter out specific headers from the incoming request
class HeaderFilterMiddleware : public AsyncMiddleware {
  public:
    void filter(const char* name) { _toRemove.push_back(name); }
    void unFilter(const char* name) { _toRemove.erase(std::remove(_toRemove.begin(), _toRemove.end(), name), _toRemove.end()); }
    void run(AsyncWebServerRequest* request, ArMiddlewareNext next) {
      for (auto it = _toRemove.begin(); it != _toRemove.end(); ++it)
        request->removeHeader(*it);
      next();
    }

  private:
    std::vector<const char*> _toRemove;
};

// curl-like logging of incoming requests
class LoggingMiddleware : public AsyncMiddleware {
  public:
    void setOutput(Print& output) { _out = &output; }
    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() { return _enabled && _out; }

    void run(AsyncWebServerRequest* request, ArMiddlewareNext next);

  private:
    Print* _out = nullptr;
    bool _enabled = true;
};

// CORS Middleware
class CorsMiddleware : public AsyncMiddleware {
  public:
    void setOrigin(const char* origin) { _origin = origin; }
    void setMethods(const char* methods) { _methods = methods; }
    void setHeaders(const char* headers) { _headers = headers; }
    void setAllowCredentials(bool credentials) { _credentials = credentials; }
    void setMaxAge(uint32_t seconds) { _maxAge = seconds; }

    void run(AsyncWebServerRequest* request, ArMiddlewareNext next);

  private:
    String _origin = "*";
    String _methods = "*";
    String _headers = "*";
    bool _credentials = true;
    uint32_t _maxAge = 86400;
};

// Rate limit Middleware
class RateLimitMiddleware : public AsyncMiddleware {
  public:
    void setMaxRequests(size_t maxRequests) { _maxRequests = maxRequests; }
    void setWindowSize(uint32_t seconds) { _windowSizeMillis = seconds * 1000; }

    bool isRequestAllowed(uint32_t& retryAfterSeconds) {
      uint32_t now = millis();

      while (!_requestTimes.empty() && _requestTimes.front() <= now - _windowSizeMillis)
        _requestTimes.pop_front();

      _requestTimes.push_back(now);

      if (_requestTimes.size() > _maxRequests) {
        _requestTimes.pop_front();
        retryAfterSeconds = (_windowSizeMillis - (now - _requestTimes.front())) / 1000 + 1;
        return false;
      }

      retryAfterSeconds = 0;
      return true;
    }

    void run(AsyncWebServerRequest* request, ArMiddlewareNext next);

  private:
    size_t _maxRequests = 0;
    uint32_t _windowSizeMillis = 0;
    std::list<uint32_t> _requestTimes;
};

/*
 * REWRITE :: One instance can be handle any Request (done by the Server)
 * */

class AsyncWebRewrite {
  protected:
    String _from;
    String _toUrl;
    String _params;
    ArRequestFilterFunction _filter{nullptr};

  public:
    AsyncWebRewrite(const char* from, const char* to) : _from(from), _toUrl(to) {
      int index = _toUrl.indexOf('?');
      if (index > 0) {
        _params = _toUrl.substring(index + 1);
        _toUrl = _toUrl.substring(0, index);
      }
    }
    virtual ~AsyncWebRewrite() {}
    AsyncWebRewrite& setFilter(ArRequestFilterFunction fn) {
      _filter = fn;
      return *this;
    }
    bool filter(AsyncWebServerRequest* request) const { return _filter == NULL || _filter(request); }
    const String& from(void) const { return _from; }
    const String& toUrl(void) const { return _toUrl; }
    const String& params(void) const { return _params; }
    virtual bool match(AsyncWebServerRequest* request) { return from() == request->url() && filter(request); }
};

/*
 * HANDLER :: One instance can be attached to any Request (done by the Server)
 * */

class AsyncWebHandler : public AsyncMiddlewareChain {
  protected:
    ArRequestFilterFunction _filter{nullptr};

  public:
    AsyncWebHandler() {}
    AsyncWebHandler& setFilter(ArRequestFilterFunction fn) {
      _filter = fn;
      return *this;
    }
    AsyncWebHandler& setAuthentication(const char* username, const char* password) {
      if (username == nullptr || password == nullptr || strlen(username) == 0 || strlen(password) == 0)
        return *this;
      AuthenticationMiddleware* m = new AuthenticationMiddleware();
      m->setUsername(username);
      m->setPassword(password);
      m->_freeOnRemoval = true;
      addMiddleware(m);
      return *this;
    };
    AsyncWebHandler& setAuthentication(const String& username, const String& password) { return setAuthentication(username.c_str(), password.c_str()); };
    bool filter(AsyncWebServerRequest* request) { return _filter == NULL || _filter(request); }
    virtual ~AsyncWebHandler() {}
    virtual bool canHandle(AsyncWebServerRequest* request __attribute__((unused))) {
      return false;
    }
    virtual void handleRequest(__unused AsyncWebServerRequest* request) {}
    virtual void handleUpload(__unused AsyncWebServerRequest* request, __unused const String& filename, __unused size_t index, __unused uint8_t* data, __unused size_t len, __unused bool final) {}
    virtual void handleBody(__unused AsyncWebServerRequest* request, __unused uint8_t* data, __unused size_t len, __unused size_t index, __unused size_t total) {}
    virtual bool isRequestHandlerTrivial() { return true; }
};

/*
 * RESPONSE :: One instance is created for each Request (attached by the Handler)
 * */

typedef enum {
  RESPONSE_SETUP,
  RESPONSE_HEADERS,
  RESPONSE_CONTENT,
  RESPONSE_WAIT_ACK,
  RESPONSE_END,
  RESPONSE_FAILED
} WebResponseState;

class AsyncWebServerResponse {
  protected:
    int _code;
    std::list<AsyncWebHeader> _headers;
    String _contentType;
    size_t _contentLength;
    bool _sendContentLength;
    bool _chunked;
    size_t _headLength;
    size_t _sentLength;
    size_t _ackedLength;
    size_t _writtenLength;
    WebResponseState _state;

  public:
#ifndef ESP8266
    static const char* responseCodeToString(int code);
#else
    static const __FlashStringHelper* responseCodeToString(int code);
#endif

  public:
    AsyncWebServerResponse();
    virtual ~AsyncWebServerResponse();
    virtual void setCode(int code);
    int code() const { return _code; }
    virtual void setContentLength(size_t len);
    void setContentType(const String& type) { setContentType(type.c_str()); }
    virtual void setContentType(const char* type);
    virtual bool addHeader(const char* name, const char* value, bool replaceExisting = true);
    bool addHeader(const String& name, const String& value, bool replaceExisting = true) { return addHeader(name.c_str(), value.c_str(), replaceExisting); }
    bool addHeader(const char* name, long value, bool replaceExisting = true) { return addHeader(name, String(value), replaceExisting); }
    bool addHeader(const String& name, long value, bool replaceExisting = true) { return addHeader(name.c_str(), value, replaceExisting); }
    virtual bool removeHeader(const char* name);
    virtual const AsyncWebHeader* getHeader(const char* name) const;
    const std::list<AsyncWebHeader>& getHeaders() const { return _headers; }

#ifndef ESP8266
    [[deprecated("Use instead: _assembleHead(String& buffer, uint8_t version)")]]
#endif
    String _assembleHead(uint8_t version) {
      String buffer;
      _assembleHead(buffer, version);
      return buffer;
    }
    virtual void _assembleHead(String& buffer, uint8_t version);

    virtual bool _started() const;
    virtual bool _finished() const;
    virtual bool _failed() const;
    virtual bool _sourceValid() const;
    virtual void _respond(AsyncWebServerRequest* request);
    virtual size_t _ack(AsyncWebServerRequest* request, size_t len, uint32_t time);
};

/*
 * SERVER :: One instance
 * */

typedef std::function<void(AsyncWebServerRequest* request)> ArRequestHandlerFunction;
typedef std::function<void(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final)> ArUploadHandlerFunction;
typedef std::function<void(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total)> ArBodyHandlerFunction;

class AsyncWebServer : public AsyncMiddlewareChain {
  protected:
    AsyncServer _server;
    std::list<std::shared_ptr<AsyncWebRewrite>> _rewrites;
    std::list<std::unique_ptr<AsyncWebHandler>> _handlers;
    AsyncCallbackWebHandler* _catchAllHandler;

  public:
    AsyncWebServer(uint16_t port);
    ~AsyncWebServer();

    void begin();
    void end();

#if ASYNC_TCP_SSL_ENABLED
    void onSslFileRequest(AcSSlFileHandler cb, void* arg);
    void beginSecure(const char* cert, const char* private_key_file, const char* password);
#endif

    AsyncWebRewrite& addRewrite(AsyncWebRewrite* rewrite);

    /**
     * @brief (compat) Add url rewrite rule by pointer
     * a deep copy of the pointer object will be created,
     * it is up to user to manage further lifetime of the object in argument
     *
     * @param rewrite pointer to rewrite object to copy setting from
     * @return AsyncWebRewrite& reference to a newly created rewrite rule
     */
    AsyncWebRewrite& addRewrite(std::shared_ptr<AsyncWebRewrite> rewrite);

    /**
     * @brief add url rewrite rule
     *
     * @param from
     * @param to
     * @return AsyncWebRewrite&
     */
    AsyncWebRewrite& rewrite(const char* from, const char* to);

    /**
     * @brief (compat) remove rewrite rule via referenced object
     * this will NOT deallocate pointed object itself, internal rule with same from/to urls will be removed if any
     * it's a compat method, better use `removeRewrite(const char* from, const char* to)`
     * @param rewrite
     * @return true
     * @return false
     */
    bool removeRewrite(AsyncWebRewrite* rewrite);

    /**
     * @brief remove rewrite rule
     *
     * @param from
     * @param to
     * @return true
     * @return false
     */
    bool removeRewrite(const char* from, const char* to);

    AsyncWebHandler& addHandler(AsyncWebHandler* handler);
    bool removeHandler(AsyncWebHandler* handler);

    AsyncCallbackWebHandler& on(const char* uri, ArRequestHandlerFunction onRequest) { return on(uri, HTTP_ANY, onRequest); }
    AsyncCallbackWebHandler& on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload = nullptr, ArBodyHandlerFunction onBody = nullptr);

    AsyncStaticWebHandler& serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control = NULL);

    void onNotFound(ArRequestHandlerFunction fn);  // called when handler is not assigned
    void onFileUpload(ArUploadHandlerFunction fn); // handle file uploads
    void onRequestBody(ArBodyHandlerFunction fn);  // handle posts with plain body content (JSON often transmitted this way as a request)

    void reset(); // remove all writers and handlers, with onNotFound/onFileUpload/onRequestBody

    void _handleDisconnect(AsyncWebServerRequest* request);
    void _attachHandler(AsyncWebServerRequest* request);
    void _rewriteRequest(AsyncWebServerRequest* request);
};

class DefaultHeaders {
    using headers_t = std::list<AsyncWebHeader>;
    headers_t _headers;

  public:
    DefaultHeaders() = default;

    using ConstIterator = headers_t::const_iterator;

    void addHeader(const String& name, const String& value) {
      _headers.emplace_back(name, value);
    }

    ConstIterator begin() const { return _headers.begin(); }
    ConstIterator end() const { return _headers.end(); }

    DefaultHeaders(DefaultHeaders const&) = delete;
    DefaultHeaders& operator=(DefaultHeaders const&) = delete;

    static DefaultHeaders& Instance() {
      static DefaultHeaders instance;
      return instance;
    }
};

#include "AsyncEventSource.h"
#include "AsyncWebSocket.h"
#include "WebHandlerImpl.h"
#include "WebResponseImpl.h"

#endif /* _AsyncWebServer_H_ */
