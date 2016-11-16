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
#include "ESPAsyncWebServer.h"
#include "WebHandlerImpl.h"


AsyncWebServer::AsyncWebServer(uint16_t port):_server(port), _rewrites(0), _handlers(0){
  _catchAllHandler = new AsyncCallbackWebHandler();
  if(_catchAllHandler == NULL)
    return;
  _server.onClient([](void *s, AsyncClient* c){
    if(c == NULL)
      return;
    c->setRxTimeout(3);
    AsyncWebServerRequest *r = new AsyncWebServerRequest((AsyncWebServer*)s, c);
    if(r == NULL){
      c->close(true);
      c->free();
      delete c;
    }
  }, this);
}

AsyncWebServer::~AsyncWebServer(){
  reset();
  delete _catchAllHandler;
}

AsyncWebRewrite& AsyncWebServer::addRewrite(AsyncWebRewrite* rewrite){
  if (_rewrites == NULL){
    _rewrites = rewrite;
  } else {
    AsyncWebRewrite *r = _rewrites;
    while(r->next != NULL) r = r->next;
    r->next = rewrite;
  }
  return *rewrite;
}

bool AsyncWebServer::removeRewrite(AsyncWebRewrite *rewrite){
  if(rewrite == _rewrites){
    _rewrites = _rewrites->next;
    return true;
  }
  AsyncWebRewrite *r = _rewrites;
  while(r != NULL){
    if(rewrite == r->next){
      AsyncWebRewrite *d = r->next;
      r->next = d->next;
      delete d;
      return true;
    }
    r = r->next;
  }
  return false;
}

AsyncWebRewrite& AsyncWebServer::rewrite(const char* from, const char* to){
  return addRewrite(new AsyncWebRewrite(from, to));
}

AsyncWebHandler& AsyncWebServer::addHandler(AsyncWebHandler* handler){
  if(_handlers == NULL){
    _handlers = handler;
  } else {
    AsyncWebHandler* h = _handlers;
    while(h->next != NULL) h = h->next;
    h->next = handler;
  }
  return *handler;
}

bool AsyncWebServer::removeHandler(AsyncWebHandler *handler){
  if(handler == _handlers){
    _handlers = _handlers->next;
    return true;
  }
  AsyncWebHandler *h = _handlers;
  while(h != NULL){
    if(handler == h->next){
      AsyncWebHandler *d = h->next;
      h->next = d->next;
      delete d;
      return true;
    }
    h = h->next;
  }
  return false;
}

void AsyncWebServer::begin(){
  _server.begin();
}

#if ASYNC_TCP_SSL_ENABLED
void AsyncWebServer::onSslFileRequest(AcSSlFileHandler cb, void* arg){
  _server.onSslFileRequest(cb, arg);
}

void AsyncWebServer::beginSecure(const char *cert, const char *key, const char *password){
  _server.beginSecure(cert, key, password);
}
#endif

void AsyncWebServer::_handleDisconnect(AsyncWebServerRequest *request){
  delete request;
}

void AsyncWebServer::_rewriteRequest(AsyncWebServerRequest *request){
  AsyncWebRewrite *r = _rewrites;
  while(r){
    if (r->from() == request->_url && r->filter(request)){
      request->_url = r->toUrl();
      request->_addGetParams(r->params());
    }
    r = r->next;
  }
}

void AsyncWebServer::_attachHandler(AsyncWebServerRequest *request){
  if(_handlers){
    AsyncWebHandler* h = _handlers;
    while(h){
      if (h->filter(request) && h->canHandle(request)){
        request->setHandler(h);
        return;
      }
      h = h->next;
    }
  }
  request->addInterestingHeader("ANY");
  request->setHandler(_catchAllHandler);
}


AsyncCallbackWebHandler& AsyncWebServer::on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody){
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->setMethod(method);
  handler->onRequest(onRequest);
  handler->onUpload(onUpload);
  handler->onBody(onBody);
  addHandler(handler);
  return *handler;
}

AsyncCallbackWebHandler& AsyncWebServer::on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload){
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->setMethod(method);
  handler->onRequest(onRequest);
  handler->onUpload(onUpload);
  addHandler(handler);
  return *handler;
}

AsyncCallbackWebHandler& AsyncWebServer::on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest){
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->setMethod(method);
  handler->onRequest(onRequest);
  addHandler(handler);
  return *handler;
}

AsyncCallbackWebHandler& AsyncWebServer::on(const char* uri, ArRequestHandlerFunction onRequest){
  AsyncCallbackWebHandler* handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->onRequest(onRequest);
  addHandler(handler);
  return *handler;
}

AsyncStaticWebHandler& AsyncWebServer::serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control){
  AsyncStaticWebHandler* handler = new AsyncStaticWebHandler(uri, fs, path, cache_control);
  addHandler(handler);
  return *handler;
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

void AsyncWebServer::reset(){
  while(_rewrites != NULL){
    AsyncWebRewrite *r = _rewrites;
    _rewrites = r->next;
    delete r;
  }
  _rewrites = NULL;
  
  while(_handlers != NULL){
    AsyncWebHandler *h = _handlers;
    _handlers = h->next;
    delete h;
  }
  _handlers = NULL;
  
  if (_catchAllHandler != NULL){
    ((AsyncCallbackWebHandler*)_catchAllHandler)->onRequest(NULL);
    ((AsyncCallbackWebHandler*)_catchAllHandler)->onUpload(NULL);
    ((AsyncCallbackWebHandler*)_catchAllHandler)->onBody(NULL);
  }
}

