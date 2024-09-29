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

class AsyncMessagePackResponse : public AsyncAbstractResponse {
  protected:
    JsonDocument _jsonBuffer;
    JsonVariant _root;
    bool _isValid;

  public:
    AsyncMessagePackResponse(bool isArray = false);

    JsonVariant& getRoot() { return _root; }
    bool _sourceValid() const { return _isValid; }
    size_t setLength();
    size_t getSize() const { return _jsonBuffer.size(); }
    size_t _fillBuffer(uint8_t* data, size_t len);
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
    AsyncCallbackMessagePackWebHandler(const String& uri, ArJsonRequestHandlerFunction onRequest = nullptr);

    void setMethod(WebRequestMethodComposite method) { _method = method; }
    void setMaxContentLength(int maxContentLength) { _maxContentLength = maxContentLength; }
    void onRequest(ArJsonRequestHandlerFunction fn) { _onRequest = fn; }
    virtual bool canHandle(AsyncWebServerRequest* request) override final;
    virtual void handleRequest(AsyncWebServerRequest* request) override final;
    virtual void handleUpload(__unused AsyncWebServerRequest* request, __unused const String& filename, __unused size_t index, __unused uint8_t* data, __unused size_t len, __unused bool final) override final {}
    virtual void handleBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) override final;
    virtual bool isRequestHandlerTrivial() override final { return _onRequest ? false : true; }
};
