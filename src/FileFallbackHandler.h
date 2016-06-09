// FileFallbackHandler.h
/*
  FileFallbackHandler Response to use with asyncwebserver
  Written by Andrew Melvin (SticilFace), based on ServeStatic, with help from me-no-dev.

  This handler will serve a 302 response to a client request for a SPIFFS file if the request comes from the STA side of the ESP network.  
  If the request comes from the AP side then it serves the file from SPIFFS. 
  This is useful if you have content that is available from a CDN but you want it to work in AP mode. 
  This also speeds things up a lot as the ESP is not left serving files, and the client can cache them as well. 


    FileFallbackHandler(SPIFFS, File_location, Uri, fallback_uri,  cache_control header (optional) ) 

 Example of callback in use

      server.addHandler( new FileFallbackHandler(SPIFFS, "/jquery/jqm1.4.5.css", "/jquery/jqm1.4.5.css", "http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css",  "max-age=86400"));


*/
#ifndef ASYNC_FILEFALLBACK_H_
#define ASYNC_FILEFALLBACK_H_

#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>

class FileFallbackHandler: public AsyncWebHandler {
  private:
    String _getPath(AsyncWebServerRequest *request); 
  protected:
    FS _fs;
    String _uri;
    String _path;
    String _forwardUri;
    String _cache_header;
    bool _isFile;
  public:
    FileFallbackHandler(FS& fs, const char* path, const char* uri, const char* forwardUri ,const char* cache_header)
      : _fs(fs), _uri(uri), _path(path), _forwardUri(forwardUri),_cache_header(cache_header){

      _isFile = _fs.exists(path) || _fs.exists((String(path)+".gz").c_str());
      if (_uri != "/" && _uri.endsWith("/")) {
        _uri = _uri.substring(0, _uri.length() - 1); 
        DEBUGF("[FileFallbackHandler] _uri / removed\n"); 
      }
      if (_path != "/" && _path.endsWith("/")) {
        _path = _path.substring(0, _path.length() - 1); 
        DEBUGF("[FileFallbackHandler] _path / removed\n"); 
      }
    }
    bool canHandle(AsyncWebServerRequest *request);
    void handleRequest(AsyncWebServerRequest *request);
    
};

#endif