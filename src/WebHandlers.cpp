/*
 * WebHandlers.cpp
 *
 *  Created on: 18.12.2015 г.
 *      Author: ficeto
 */
#include "ESPAsyncWebServer.h"
#include "AsyncWebServerHandlerImpl.h"

bool AsyncStaticWebHandler::canHandle(AsyncWebServerRequest *request){
  if(request->method() != HTTP_GET)
    return false;
  if ((_isFile && request->url() != _uri) || !request->url().startsWith(_uri))
    return false;
  return true;
}

void AsyncStaticWebHandler::handleRequest(AsyncWebServerRequest *request){
  String path = request->url();
  if(!_isFile){
    if(_uri != "/"){
      path = path.substring(_uri.length());
      if(!path.length())
        path = "/";
    }
    if(path.endsWith("/"))
      path += "index.htm";
  }

  if(_fs.exists(path) || _fs.exists(path+".gz")){
    AsyncWebServerResponse * response = request->beginResponse(_fs, path);
    if (_cache_header.length() != 0)
      response->addHeader("Cache-Control", _cache_header);
    request->send(response);
  } else
    request->send(404);
  path = String();
}


