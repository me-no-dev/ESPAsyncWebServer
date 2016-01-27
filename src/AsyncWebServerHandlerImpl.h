/*
 * AsyncWebServerHandlerImpl.h
 *
 *  Created on: 19.12.2015 г.
 *      Author: ficeto
 */

#ifndef ASYNCWEBSERVERHANDLERIMPL_H_
#define ASYNCWEBSERVERHANDLERIMPL_H_


#include "stddef.h"

class AsyncStaticWebHandler: public AsyncWebHandler {
  private:
  protected:
    FS _fs;
    String _uri;
    String _path;
    String _cache_header;
    bool _isFile;
  public:
    AsyncStaticWebHandler(FS& fs, const char* path, const char* uri, const char* cache_header)
      : _fs(fs), _uri(uri), _path(path), _cache_header(cache_header){

      _isFile = _fs.exists(path) || _fs.exists((String(path)+".gz").c_str());
      if (_uri != "/" && _uri.endsWith("/")) {
        _uri = _uri.substring(0, _uri.length() - 1); 
        DEBUGF("[AsyncStaticWebHandler] _uri / removed"); 
      }
      if (_path != "/" && _path.endsWith("/")) {
        _path = _path.substring(0, _path.length() - 1); 
        DEBUGF("[AsyncStaticWebHandler] _path / removed"); 
      }


    }
    bool canHandle(AsyncWebServerRequest *request);
    void handleRequest(AsyncWebServerRequest *request);
};

class AsyncCallbackWebHandler: public AsyncWebHandler {
  private:
  protected:
    String _uri;
    WebRequestMethod _method;
    ArRequestHandlerFunction _onRequest;
    ArUploadHandlerFunction _onUpload;
    ArBodyHandlerFunction _onBody;
  public:
    AsyncCallbackWebHandler() : _uri(), _method(HTTP_ANY), _onRequest(NULL), _onUpload(NULL), _onBody(NULL){}
    void setUri(String uri){ _uri = uri; }
    void setMethod(WebRequestMethod method){ _method = method; }
    void onRequest(ArRequestHandlerFunction fn){ _onRequest = fn; }
    void onUpload(ArUploadHandlerFunction fn){ _onUpload = fn; }
    void onBody(ArBodyHandlerFunction fn){ _onBody = fn; }

    bool canHandle(AsyncWebServerRequest *request){
      if(!_onRequest)
        return false;

      if(_method != HTTP_ANY && request->method() != _method)
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
