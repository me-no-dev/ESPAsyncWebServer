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

bool AsyncStaticWebHandler::canHandle(AsyncWebServerRequest *request)
{
  if (request->method() != HTTP_GET) {
    return false;
  }
  if ((_isFile && request->url() != _uri) ) {
    return false;
  }
  //  if the root of the request matches the _uri then it checks to see if there is a file it can handle. 
  if (request->url().startsWith(_uri)) {
    String path = _getPath(request);
    if (_fs.exists(path) || _fs.exists(path + ".gz")) {
       DEBUGF("[AsyncStaticWebHandler::canHandle] TRUE\n");
      return true;
    }
  }

  return false;
}

String AsyncStaticWebHandler::_getPath(AsyncWebServerRequest *request)
{

  String path = request->url();
  DEBUGF("[AsyncStaticWebHandler::_getPath]\n");
  DEBUGF("  [stored] _uri = %s, _path = %s\n" , _uri.c_str(), _path.c_str() ) ;
  DEBUGF("  [request] url = %s\n", request->url().c_str() );

  if (!_isFile) {
    DEBUGF("   _isFile = false\n");
    String baserequestUrl = request->url().substring(_uri.length());  // this is the request - stored _uri...  /espman/
    DEBUGF("  baserequestUrl = %s\n", baserequestUrl.c_str());

    if (!baserequestUrl.length()) {
      baserequestUrl += "/";
    }

    path = _path + baserequestUrl;
    DEBUGF("  path = path + baserequestUrl, path = %s\n", path.c_str());

    if (path.endsWith("/")) {
      DEBUGF("  3 path ends with / : path = index.htm \n");
      path += "index.htm";
    }
  } else {
    path = _path;
  }

  DEBUGF(" final path = %s\n", path.c_str());
  DEBUGF("[AsyncStaticWebHandler::_getPath] END\n\n");

  return path;
}


void AsyncStaticWebHandler::handleRequest(AsyncWebServerRequest *request)
{

  String path = _getPath(request);

  if (_fs.exists(path) || _fs.exists(path + ".gz")) {
    AsyncWebServerResponse * response = request->beginResponse(_fs, path);
    if (_cache_header.length() != 0)
      response->addHeader("Cache-Control", _cache_header);
    request->send(response);
  } else {
    request->send(404);
  }
  path = String();


}
