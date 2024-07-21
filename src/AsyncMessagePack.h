#pragma once

/*
   server.on("/msg_pack", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncMessagePackResponse * response = new AsyncMessagePackResponse();
    JsonObject& root = response->getRoot();
    root["key1"] = "key number one";
    JsonObject& nested = root.createNestedObject("nested");
    nested["key1"] = "key number one";
    response->setLength();
    request->send(response);
  });

  --------------------

  AsyncCallbackMessagePackWebHandler* handler = new AsyncCallbackMessagePackWebHandler("/msg_pack/endpoint");
  handler->onRequest([](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();
    // ...
  });
  server.addHandler(handler);
*/

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include "ChunkPrint.h"
#include "literals.h"

class AsyncMessagePackResponse : public AsyncAbstractResponse {
  protected:
    JsonDocument _jsonBuffer;
    JsonVariant _root;
    bool _isValid;

  public:
    AsyncMessagePackResponse(bool isArray = false) : _isValid{false} {
      _code = 200;
      _contentType = asyncsrv::T_application_msgpack;
      if (isArray)
        _root = _jsonBuffer.add<JsonArray>();
      else
        _root = _jsonBuffer.add<JsonObject>();
    }

    JsonVariant& getRoot() { return _root; }

    bool _sourceValid() const { return _isValid; }

    size_t setLength() {
      _contentLength = measureMsgPack(_root);
      if (_contentLength) {
        _isValid = true;
      }
      return _contentLength;
    }

    size_t getSize() const { return _jsonBuffer.size(); }

    size_t _fillBuffer(uint8_t* data, size_t len) {
      ChunkPrint dest(data, _sentLength, len);
      serializeMsgPack(_root, dest);
      return len;
    }
};

class AsyncCallbackMessagePackWebHandler : public AsyncWebHandler {
  public:
    typedef std::function<void(AsyncWebServerRequest* request, JsonVariant& json)> ArJsonRequestHandlerFunction;

  protected:
    const String _uri;
    WebRequestMethodComposite _method;
    ArJsonRequestHandlerFunction _onRequest;
    size_t _contentLength;
    size_t _maxContentLength;

  public:
    AsyncCallbackMessagePackWebHandler(const String& uri, ArJsonRequestHandlerFunction onRequest = nullptr)
        : _uri(uri), _method(HTTP_GET | HTTP_POST | HTTP_PUT | HTTP_PATCH), _onRequest(onRequest), _maxContentLength(16384) {}

    void setMethod(WebRequestMethodComposite method) { _method = method; }
    void setMaxContentLength(int maxContentLength) { _maxContentLength = maxContentLength; }
    void onRequest(ArJsonRequestHandlerFunction fn) { _onRequest = fn; }

    virtual bool canHandle(AsyncWebServerRequest* request) override final {
      if (!_onRequest)
        return false;

      WebRequestMethodComposite request_method = request->method();
      if (!(_method & request_method))
        return false;

      if (_uri.length() && (_uri != request->url() && !request->url().startsWith(_uri + "/")))
        return false;

      if (request_method != HTTP_GET && !request->contentType().equalsIgnoreCase(asyncsrv::T_application_msgpack))
        return false;

      request->addInterestingHeader("ANY");
      return true;
    }

    virtual void handleRequest(AsyncWebServerRequest* request) override final {
      if ((_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str()))
        return request->requestAuthentication();

      if (_onRequest) {
        if (request->method() == HTTP_GET) {
          JsonVariant json;
          _onRequest(request, json);
          return;

        } else if (request->_tempObject != NULL) {
          JsonDocument jsonBuffer;
          DeserializationError error = deserializeMsgPack(jsonBuffer, (uint8_t*)(request->_tempObject));

          if (!error) {
            JsonVariant json = jsonBuffer.as<JsonVariant>();
            _onRequest(request, json);
            return;
          }
        }
        request->send(_contentLength > _maxContentLength ? 413 : 400);
      } else {
        request->send(500);
      }
    }

    virtual void handleUpload(__unused AsyncWebServerRequest* request, __unused const String& filename, __unused size_t index, __unused uint8_t* data, __unused size_t len, __unused bool final) override final {}

    virtual void handleBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) override final {
      if (_onRequest) {
        _contentLength = total;
        if (total > 0 && request->_tempObject == NULL && total < _maxContentLength) {
          request->_tempObject = malloc(total);
        }
        if (request->_tempObject != NULL) {
          memcpy((uint8_t*)(request->_tempObject) + index, data, len);
        }
      }
    }

    virtual bool isRequestHandlerTrivial() override final { return _onRequest ? false : true; }
};
