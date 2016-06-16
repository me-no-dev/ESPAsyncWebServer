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
  if (request->method() == HTTP_GET &&
      request->url().startsWith(_uri) &&
      _getPath(request, true).length()) {

    DEBUGF("[AsyncStaticWebHandler::canHandle] TRUE\n");
    return true;

  }

  return false;
}

String AsyncStaticWebHandler::_getPath(AsyncWebServerRequest *request, const bool withStats)
{
  // Remove the found uri
  String path = request->url().substring(_uri.length());

  // We can skip the file check if we serving a directory and (we have full match or we end with '/')
  bool canSkipFileCheck = _isDir && (path.length() == 0 || path[path.length()-1] == '/');

  path = _path + path;

  // Do we have a file or .gz file
  if (!canSkipFileCheck) if (_fileExists(path, withStats)) return path;

  // Try to add default page, ensure there is a trailing '/' ot the path.
  if (path.length() == 0 || path[path.length()-1] != '/') path += "/";
  path += "index.htm";

  if (_fileExists(path, withStats)) return path;

  // No file - return empty string
  return String();
}

bool AsyncStaticWebHandler::_fileExists(const String path, const bool withStats)
{
  bool fileFound = false;
  bool gzipFound = false;

  String gzip = path + ".gz";

  if (_gzipFirst) {
    gzipFound = _fs.exists(gzip);
    if (!gzipFound) fileFound = _fs.exists(path);
  } else {
    fileFound = _fs.exists(path);
    if (!fileFound) gzipFound =  _fs.exists(gzip);
  }

  bool found = fileFound || gzipFound;

  if (withStats && found) {
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
  String path = _getPath(request, false);

  if (path.length()) {
    AsyncWebServerResponse * response = new AsyncFileResponse(_fs, path);
    if (_cache_header.length() != 0)
      response->addHeader("Cache-Control", _cache_header);
    request->send(response);
  } else {
    request->send(404);
  }
  path = String();
}
