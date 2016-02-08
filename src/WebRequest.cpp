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
#include <libb64/cencode.h>
#include <WebResponseImpl.h>

#ifndef ESP8266
#define os_strlen strlen
#endif

AsyncWebServerRequest::AsyncWebServerRequest(AsyncWebServer* s, AsyncClient* c)
  : _client(c)
  , _server(s)
  , _handler(NULL)
  , _response(NULL)
  , _interestingHeaders(new StringArray())
  , _temp()
  , _parseState(0)
  , _version(0)
  , _method(HTTP_ANY)
  , _url()
  , _host()
  , _contentType()
  , _boundary()
  , _authorization()
  , _isMultipart(false)
  , _isPlainPost(false)
  , _expectingContinue(false)
  , _contentLength(0)
  , _parsedLength(0)
  , _headers(NULL)
  , _params(NULL)
  , _multiParseState(0)
  , _boundaryPosition(0)
  , _itemStartIndex(0)
  , _itemSize(0)
  , _itemName()
  , _itemFilename()
  , _itemType()
  , _itemValue()
  , _itemBuffer(0)
  , _itemBufferIndex(0)
  , _itemIsFile(false)
  , next(NULL)
{
  c->onError([](void *r, AsyncClient* c, int8_t error){ AsyncWebServerRequest *req = (AsyncWebServerRequest*)r; req->_onError(error); }, this);
  c->onAck([](void *r, AsyncClient* c, size_t len, uint32_t time){ AsyncWebServerRequest *req = (AsyncWebServerRequest*)r; req->_onAck(len, time); }, this);
  c->onDisconnect([](void *r, AsyncClient* c){ AsyncWebServerRequest *req = (AsyncWebServerRequest*)r; req->_onDisconnect(); }, this);
  c->onTimeout([](void *r, AsyncClient* c, uint32_t time){ AsyncWebServerRequest *req = (AsyncWebServerRequest*)r; req->_onTimeout(time); }, this);
  c->onData([](void *r, AsyncClient* c, void *buf, size_t len){ AsyncWebServerRequest *req = (AsyncWebServerRequest*)r; req->_onData(buf, len); }, this);
  c->onPoll([](void *r, AsyncClient* c){ AsyncWebServerRequest *req = (AsyncWebServerRequest*)r; req->_onPoll(); }, this);
}

AsyncWebServerRequest::~AsyncWebServerRequest(){
  while(_headers != NULL){
    AsyncWebHeader *h = _headers;
    _headers = h->next;
    delete h;
  }

  while(_params != NULL){
    AsyncWebParameter *p = _params;
    _params = p->next;
    delete p;
  }

  _interestingHeaders->free();
  delete _interestingHeaders;

  if(_response != NULL){
    delete _response;
  }

}

void AsyncWebServerRequest::_onPoll(){
  //os_printf("p\n");
  if(_response != NULL && !_response->_finished() && _client->canSend()){
    _response->_ack(this, 0, 0);
  }
}

void AsyncWebServerRequest::_onAck(size_t len, uint32_t time){
  //os_printf("a:%u:%u\n", len, time);
  if(_response != NULL){
    if(!_response->_finished()){
      _response->_ack(this, len, time);
    } else {
      AsyncWebServerResponse* r = _response;
      _response = NULL;
      delete r;
    }
  }
}

void AsyncWebServerRequest::_onError(int8_t error){
  if(error != -11)
    os_printf("ERROR[%d] %s, state: %s\n", error, _client->errorToString(error), _client->stateToString());
}

void AsyncWebServerRequest::_onTimeout(uint32_t time){
  os_printf("TIMEOUT: %u, state: %s\n", time, _client->stateToString());
  _client->close();
}

void AsyncWebServerRequest::_onDisconnect(){
  //os_printf("d\n");
  _client->free();
  delete _client;
  _server->_handleDisconnect(this);
}

void AsyncWebServerRequest::_addParam(AsyncWebParameter *p){
  if(_params == NULL)
    _params = p;
  else {
    AsyncWebParameter *ps = _params;
    while(ps->next != NULL) ps = ps->next;
    ps->next = p;
  }
}

int AsyncWebServerRequest::headers(){
  int i = 0;
  AsyncWebHeader* h = _headers;
  while(h != NULL){
    i++; h = h->next;
  }
  return i;
}

bool AsyncWebServerRequest::hasHeader(String name){
  AsyncWebHeader* h = _headers;
  while(h != NULL){
    if(h->name() == name)
      return true;
    h = h->next;
  }
  return false;
}

AsyncWebHeader* AsyncWebServerRequest::getHeader(String name){
  AsyncWebHeader* h = _headers;
  while(h != NULL){
    if(h->name() == name)
      return h;
    h = h->next;
  }
  return NULL;
}

AsyncWebHeader* AsyncWebServerRequest::getHeader(int num){
  int i = 0;
  AsyncWebHeader* h = _headers;
  while(h != NULL){
    if(num == i++)
      return h;
    h = h->next;
  }
  return NULL;
}

int AsyncWebServerRequest::params(){
  int i = 0;
  AsyncWebParameter* h = _params;
  while(h != NULL){
    i++; h = h->next;
  }
  return i;
}

bool AsyncWebServerRequest::hasParam(String name, bool post, bool file){
  AsyncWebParameter* h = _params;
  while(h != NULL){
    if(h->name() == name && h->isPost() == post && h->isFile() == file)
      return true;
    h = h->next;
  }
  return false;
}

AsyncWebParameter* AsyncWebServerRequest::getParam(String name, bool post, bool file){
  AsyncWebParameter* h = _params;
  while(h != NULL){
    if(h->name() == name && h->isPost() == post && h->isFile() == file)
      return h;
    h = h->next;
  }
  return NULL;
}

AsyncWebParameter* AsyncWebServerRequest::getParam(int num){
  int i = 0;
  AsyncWebParameter* h = _params;
  while(h != NULL){
    if(num == i++)
      return h;
    h = h->next;
  }
  return NULL;
}

void AsyncWebServerRequest::addInterestingHeader(String name){
  if(!_interestingHeaders->contains(name)) _interestingHeaders->add(name);
}


void AsyncWebServerRequest::send(AsyncWebServerResponse *response){
  _response = response;
  _response->_respond(this);
}



AsyncWebServerResponse * AsyncWebServerRequest::beginResponse(int code, String contentType, String content){
  return new AsyncBasicResponse(code, contentType, content);
}

AsyncWebServerResponse * AsyncWebServerRequest::beginResponse(FS &fs, String path, String contentType, bool download){
  if(fs.exists(path) || (!download && fs.exists(path+".gz")))
    return new AsyncFileResponse(fs, path, contentType, download);
  return NULL;
}

AsyncWebServerResponse * AsyncWebServerRequest::beginResponse(Stream &stream, String contentType, size_t len){
  return new AsyncStreamResponse(stream, contentType, len);
}

AsyncWebServerResponse * AsyncWebServerRequest::beginResponse(String contentType, size_t len, AwsResponseFiller callback){
  return new AsyncCallbackResponse(contentType, len, callback);
}

AsyncWebServerResponse * AsyncWebServerRequest::beginChunkedResponse(String contentType, AwsResponseFiller callback){
  if(_version)
    return new AsyncChunkedResponse(contentType, callback);
  return new AsyncCallbackResponse(contentType, 0, callback);
}

AsyncResponseStream * AsyncWebServerRequest::beginResponseStream(String contentType, size_t bufferSize){
  return new AsyncResponseStream(contentType, bufferSize);
}

void AsyncWebServerRequest::send(int code, String contentType, String content){
  send(beginResponse(code, contentType, content));
}

void AsyncWebServerRequest::send(FS &fs, String path, String contentType, bool download){
  if(fs.exists(path) || (!download && fs.exists(path+".gz"))){
    send(beginResponse(fs, path, contentType, download));
  } else send(404);
}

void AsyncWebServerRequest::send(Stream &stream, String contentType, size_t len){
  send(beginResponse(stream, contentType, len));
}

void AsyncWebServerRequest::send(String contentType, size_t len, AwsResponseFiller callback){
  send(beginResponse(contentType, len, callback));
}

void AsyncWebServerRequest::sendChunked(String contentType, AwsResponseFiller callback){
  send(beginChunkedResponse(contentType, callback));
}


bool AsyncWebServerRequest::authenticate(const char * username, const char * password){
  if(_authorization.length()){
      char toencodeLen = os_strlen(username)+os_strlen(password)+1;
      char *toencode = new char[toencodeLen];
      if(toencode == NULL){
        return false;
      }
      char *encoded = new char[base64_encode_expected_len(toencodeLen)+1];
      if(encoded == NULL){
        delete[] toencode;
        return false;
      }
      sprintf(toencode, "%s:%s", username, password);
      if(base64_encode_chars(toencode, toencodeLen, encoded) > 0 && _authorization.equals(encoded)){
        delete[] toencode;
        delete[] encoded;
        return true;
      }
      delete[] toencode;
      delete[] encoded;
  }
  return false;
}

bool AsyncWebServerRequest::authenticate(const char * hash){
  return (_authorization.length() && (_authorization == String(hash)));
}

void AsyncWebServerRequest::requestAuthentication(){
  AsyncWebServerResponse * r = beginResponse(401);
  r->addHeader("WWW-Authenticate", "Basic realm=\"Login Required\"");
  send(r);
}

bool AsyncWebServerRequest::hasArg(const char* name){
  AsyncWebParameter* h = _params;
  while(h != NULL){
    if(h->name() == String(name))
      return true;
    h = h->next;
  }
  return false;
}

String AsyncWebServerRequest::arg(const char* name){
  AsyncWebParameter* h = _params;
  while(h != NULL){
    if(h->name() == String(name))
      return h->value();
    h = h->next;
  }
  return String();
}

String AsyncWebServerRequest::arg(int i){
  return getParam(i)->value();
}

String AsyncWebServerRequest::argName(int i){
  return getParam(i)->name();
}

String AsyncWebServerRequest::header(const char* name){
  AsyncWebHeader* h = getHeader(String(name));
  if(h)
    return h->value();
  return String();
}

String AsyncWebServerRequest::header(int i){
  AsyncWebHeader* h = getHeader(i);
  if(h)
    return h->value();
  return String();
}

String AsyncWebServerRequest::headerName(int i){
  AsyncWebHeader* h = getHeader(i);
  if(h)
    return h->name();
  return String();
}

bool AsyncWebServerRequest::hasHeader(const char* name){
  return hasHeader(String(name));
}


String AsyncWebServerRequest::urlDecode(const String& text){
  String decoded = "";
  char temp[] = "0x00";
  unsigned int len = text.length();
  unsigned int i = 0;
  while (i < len){
    char decodedChar;
    char encodedChar = text.charAt(i++);
    if ((encodedChar == '%') && (i + 1 < len)){
      temp[2] = text.charAt(i++);
      temp[3] = text.charAt(i++);

      decodedChar = strtol(temp, NULL, 16);
    } else {
      if (encodedChar == '+'){
        decodedChar = ' ';
      } else {
        decodedChar = encodedChar;  // normal ascii char
      }
    }
    decoded += decodedChar;
  }
  return decoded;
}


const char * AsyncWebServerRequest::methodToString(){
  if(_method == HTTP_ANY) return "ANY";
  else if(_method == HTTP_GET) return "GET";
  else if(_method == HTTP_POST) return "POST";
  else if(_method == HTTP_DELETE) return "DELETE";
  else if(_method == HTTP_PUT) return "PUT";
  else if(_method == HTTP_PATCH) return "PATCH";
  else if(_method == HTTP_HEAD) return "HEAD";
  else if(_method == HTTP_OPTIONS) return "OPTIONS";
  return "UNKNOWN";
}
