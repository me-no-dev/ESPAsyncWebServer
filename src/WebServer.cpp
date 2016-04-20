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


AsyncWebServer::AsyncWebServer(uint16_t port):_server(port), _handlers(0){
  _catchAllHandler = new AsyncCallbackWebHandler();
  if(_catchAllHandler == NULL)
    return;
  _server.onClient([](void *s, AsyncClient* c){
    if(c == NULL)
      return;
    AsyncWebServerRequest *r = new AsyncWebServerRequest((AsyncWebServer*)s, c);
    if(r == NULL){
      c->close(true);
      c->free();
      delete c;
    }
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
  request->addInterestingHeader("ANY");
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
