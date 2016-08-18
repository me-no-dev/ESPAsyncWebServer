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
#ifndef ASYNCWEBSERVERHANDLERIMPL_H_
#define ASYNCWEBSERVERHANDLERIMPL_H_


#include "stddef.h"
#include <time.h>

class AsyncStaticWebHandler: public AsyncWebHandler {
  private:
    bool _getFile(AsyncWebServerRequest *request);
    bool _fileExists(AsyncWebServerRequest *request, const String path);
    uint8_t _countBits(const uint8_t value);
  protected:
    FS _fs;
    String _uri;
    String _path;
    String _default_file;
    String _cache_control;
    String _last_modified;
    bool _isDir;
    bool _gzipFirst;
    uint8_t _gzipStats;
  public:
    AsyncStaticWebHandler(const char* uri, FS& fs, const char* path, const char* cache_control);
    bool canHandle(AsyncWebServerRequest *request);
    void handleRequest(AsyncWebServerRequest *request);
    AsyncStaticWebHandler& setIsDir(bool isDir);
    AsyncStaticWebHandler& setDefaultFile(const char* filename);
    AsyncStaticWebHandler& setCacheControl(const char* cache_control);
    AsyncStaticWebHandler& setLastModified(const char* last_modified);
    AsyncStaticWebHandler& setLastModified(struct tm* last_modified);
  #ifdef ESP8266
    AsyncStaticWebHandler& setLastModified(time_t last_modified);
    AsyncStaticWebHandler& setLastModified(); //sets to current time. Make sure sntp is runing and time is updated
  #endif
};

class AsyncCallbackWebHandler: public AsyncWebHandler {
  private:
  protected:
    String _uri;
    WebRequestMethodComposite _method;
    ArRequestHandlerFunction _onRequest;
    ArUploadHandlerFunction _onUpload;
    ArBodyHandlerFunction _onBody;
  public:
    AsyncCallbackWebHandler() : _uri(), _method(HTTP_ANY), _onRequest(NULL), _onUpload(NULL), _onBody(NULL){}
    void setUri(String uri){ _uri = uri; }
    void setMethod(WebRequestMethodComposite method){ _method = method; }
    void onRequest(ArRequestHandlerFunction fn){ _onRequest = fn; }
    void onUpload(ArUploadHandlerFunction fn){ _onUpload = fn; }
    void onBody(ArBodyHandlerFunction fn){ _onBody = fn; }

    bool canHandle(AsyncWebServerRequest *request){
      if(!_onRequest)
        return false;

      if(!(_method & request->method()))
        return false;

      if(_uri.length() && (_uri != request->url() && !request->url().startsWith(_uri+"/")))
        return false;

      request->addInterestingHeader("ANY");
      return true;
    }
    void handleRequest(AsyncWebServerRequest *request){
      if(_onRequest)
        _onRequest(request);
      else
        request->send(500);
    }
    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if(_onUpload)
        _onUpload(request, filename, index, data, len, final);
    }
    void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if(_onBody)
        _onBody(request, data, len, index, total);
    }
};

#endif /* ASYNCWEBSERVERHANDLERIMPL_H_ */
