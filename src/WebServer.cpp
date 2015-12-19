#include "ESPAsyncWebServer.h"
#include "AsyncWebServerHandlerImpl.h"


AsyncWebServer::AsyncWebServer(uint16_t port):_server(port), _handlers(0), _catchAllHandler(new AsyncCallbackWebHandler()){
  _server.onClient([](void *s, AsyncClient* c){
    new AsyncWebServerRequest((AsyncWebServer*)s, c);
  }, this);
}

void AsyncWebServer::addHandler(AsyncWebHandler* handler){
  if(_handlers == NULL){
    _handlers = handler;
  } else {
    AsyncWebHandler* h = _handlers;
    while(h->next != NULL) h = h->next;
    h->next = handler;
  }
}

void AsyncWebServer::begin(){
  _server.begin();
}

void AsyncWebServer::_handleDisconnect(AsyncWebServerRequest *request){
  delete request;
}

void AsyncWebServer::_handleRequest(AsyncWebServerRequest *request){
  if(_handlers){
    AsyncWebHandler* h = _handlers;
    while(h && !h->canHandle(request)) h = h->next;
    if(h){
      request->setHandler(h);
      return;
    }
  }
  request->setHandler(_catchAllHandler);
}


void AsyncWebServer::on(const char* uri, WebRequestMethod method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody){
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->setMethod(method);
  handler->onRequest(onRequest);
  handler->onUpload(onUpload);
  handler->onBody(onBody);
  addHandler(handler);
}

void AsyncWebServer::on(const char* uri, WebRequestMethod method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload){
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->setMethod(method);
  handler->onRequest(onRequest);
  handler->onUpload(onUpload);
  addHandler(handler);
}

void AsyncWebServer::on(const char* uri, WebRequestMethod method, ArRequestHandlerFunction onRequest){
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->setMethod(method);
  handler->onRequest(onRequest);
  addHandler(handler);
}

void AsyncWebServer::on(const char* uri, ArRequestHandlerFunction onRequest){
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->onRequest(onRequest);
  addHandler(handler);
}

void AsyncWebServer::serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_header){
  addHandler(new AsyncStaticWebHandler(fs, path, uri, cache_header));
}

void AsyncWebServer::onNotFound(ArRequestHandlerFunction fn){
  ((AsyncCallbackWebHandler*)_catchAllHandler)->onRequest(fn);
}

void AsyncWebServer::onFileUpload(ArUploadHandlerFunction fn){
  ((AsyncCallbackWebHandler*)_catchAllHandler)->onUpload(fn);
}

void AsyncWebServer::onRequestBody(ArBodyHandlerFunction fn){
  ((AsyncCallbackWebHandler*)_catchAllHandler)->onBody(fn);
}


