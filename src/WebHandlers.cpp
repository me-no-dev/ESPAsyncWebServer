/*
 * WebHandlers.cpp
 *
 *  Created on: 18.12.2015 г.
 *      Author: ficeto
 */
#include "ESPAsyncWebServer.h"
#include "AsyncWebServerHandlerImpl.h"

bool AsyncStaticWebHandler::canHandle(AsyncWebServerRequest *request)
{
  if (request->method() != HTTP_GET) {
    return false;
  }
  if ((_isFile && request->url() != _uri) || !request->url().startsWith(_uri)) {
    return false;
  }
  return true;
}

void AsyncStaticWebHandler::handleRequest(AsyncWebServerRequest *request)
{
  String path = request->url();
  DEBUGF("[AsyncStaticWebHandler::handleRequest]\n");
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
   
  DEBUGF("[AsyncStaticWebHandler::handleRequest] final path = %s\n", path.c_str());

  if (_fs.exists(path) || _fs.exists(path + ".gz")) {
    AsyncWebServerResponse * response = request->beginResponse(_fs, path);
    if (_cache_header.length() != 0)
      response->addHeader("Cache-Control", _cache_header);
    request->send(response);
  } else {
    request->send(404);
  }
  path = String();

  DEBUGF("[AsyncStaticWebHandler::handleRequest] END\n\n");

}
