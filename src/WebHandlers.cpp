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

AsyncStaticWebHandler::AsyncStaticWebHandler(FS& fs, const char* path, const char* uri, const char* cache_header)
  : _fs(fs), _uri(uri), _path(path), _cache_header(cache_header)
{
  // Ensure leading '/'
  if (_uri.length() == 0 || _uri[0] != '/') _uri = "/" + _uri;
  if (_path.length() == 0 || _path[0] != '/') _path = "/" + _path;

  // If path ends with '/' we assume a hint that this is a directory to improve performance.
  // However - if it does not end with '/' we, can't assume a file, path can still be a directory.
  _isDir = _path[_path.length()-1] == '/';

  // Remove the trailing '/' so we can handle default file
  // Notice that root will be "" not "/"
  if (_uri[_uri.length()-1] == '/') _uri = _uri.substring(0, _uri.length()-1);
  if (_path[_path.length()-1] == '/') _path = _path.substring(0, _path.length()-1);

  // Reset stats
  _gzipFirst = false;
  _gzipStats = 0;
  _fileStats = 0;
}

bool AsyncStaticWebHandler::canHandle(AsyncWebServerRequest *request)
{
  if (request->method() == HTTP_GET &&
      request->url().startsWith(_uri) &&
      _getFile(request)) {

    DEBUGF("[AsyncStaticWebHandler::canHandle] TRUE\n");
    return true;
  }

  return false;
}

bool AsyncStaticWebHandler::_getFile(AsyncWebServerRequest *request)
{
  // Remove the found uri
  String path = request->url().substring(_uri.length());

  // We can skip the file check and look for default if request is to the root of a directory or that request path ends with '/'
  bool canSkipFileCheck = (_isDir && path.length() == 0) || (path.length() && path[path.length()-1] == '/');

  path = _path + path;

  // Do we have a file or .gz file
  if (!canSkipFileCheck && _fileExists(request, path))
    return true;

  // Try to add default page, ensure there is a trailing '/' ot the path.
  if (path.length() == 0 || path[path.length()-1] != '/')
    path += "/";
  path += "index.htm";

  return _fileExists(request, path);
}

bool AsyncStaticWebHandler::_fileExists(AsyncWebServerRequest *request, const String path)
{
  bool fileFound = false;
  bool gzipFound = false;

  String gzip = path + ".gz";

  if (_gzipFirst) {
    request->_tempFile = _fs.open(gzip, "r");
    gzipFound = request->_tempFile == true;
    if (!gzipFound){
      request->_tempFile = _fs.open(path, "r");
      fileFound = request->_tempFile == true;
    }
  } else {
    request->_tempFile = _fs.open(path, "r");
    fileFound = request->_tempFile == true;
    if (!fileFound){
      request->_tempFile = _fs.open(gzip, "r");
      gzipFound = request->_tempFile == true;
    }
  }

  bool found = fileFound || gzipFound;

  if (found) {
    size_t plen = path.length();
    char * _tempPath = (char*)malloc(plen+1);
    snprintf(_tempPath, plen+1, "%s", path.c_str());
    request->_tempObject = (void*)_tempPath;
    _gzipStats = (_gzipStats << 1) + gzipFound ? 1 : 0;
    _fileStats = (_fileStats << 1) + fileFound ? 1 : 0;
    _gzipFirst = _countBits(_gzipStats) > _countBits(_fileStats);
  }

  return found;
}

uint8_t AsyncStaticWebHandler::_countBits(const uint8_t value)
{
  uint8_t w = value;
  uint8_t n;
  for (n=0; w!=0; n++) w&=w-1;
  return n;
}

void AsyncStaticWebHandler::handleRequest(AsyncWebServerRequest *request)
{
  if (request->_tempFile == true) {
    AsyncWebServerResponse * response = new AsyncFileResponse(request->_tempFile, String((char*)request->_tempObject));
    free(request->_tempObject);
    request->_tempObject = NULL;
    if (_cache_header.length() != 0)
      response->addHeader("Cache-Control", _cache_header);
    request->send(response);
  } else {
    request->send(404);
  }
}
