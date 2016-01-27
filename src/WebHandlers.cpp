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
//  os_printf("[AsyncStaticWebHandler::handleRequest]\n");
//  os_printf("  [stored] _uri = %s, _path = %s\n" , _uri.c_str(), _path.c_str() ) ;
//  os_printf("  [request] url = %s\n", request->url().c_str() );

  if (!_isFile) {
      //os_printf("   _isFile = false\n");
      String baserequestUrl = request->url().substring(_uri.length());  // this is the request - stored _uri...  /espman/ 
      //os_printf("  baserequestUrl = %s\n", baserequestUrl.c_str()); 

      if (baserequestUrl.length()) {
        path = _path + baserequestUrl;
        //os_printf("  baserequestUrl length > 0: path = path + baserequestUrl, path = %s\n", path.c_str()); 
      }
    if (path.endsWith("/")) {
        //os_printf("  3 path ends with / : path = index.htm \n");
      path += "index.htm";
    }
  } else {
    path = _path;
  }
   
//  os_printf("[AsyncStaticWebHandler::handleRequest] final path = %s\n", path.c_str());

  if (_fs.exists(path) || _fs.exists(path + ".gz")) {
    AsyncWebServerResponse * response = request->beginResponse(_fs, path);
    if (_cache_header.length() != 0)
      response->addHeader("Cache-Control", _cache_header);
    request->send(response);
  } else {
    request->send(404);
  }
  path = String();

//  os_printf("[AsyncStaticWebHandler::handleRequest] END\n\n");

}
