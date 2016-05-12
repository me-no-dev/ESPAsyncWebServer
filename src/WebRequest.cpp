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
#include "WebResponseImpl.h"

#ifndef ESP8266
#define os_strlen strlen
#endif

#define __is_param_char(c) ((c) && ((c) != '{') && ((c) != '[') && ((c) != '&') && ((c) != '='))

enum { PARSE_REQ_START, PARSE_REQ_HEADERS, PARSE_REQ_BODY, PARSE_REQ_END, PARSE_REQ_FAIL };

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

void AsyncWebServerRequest::_onData(void *buf, size_t len){
  if(_parseState < PARSE_REQ_BODY){
    size_t i;
    for(i=0; i<len; i++){
      if(_parseState < PARSE_REQ_BODY)
        _parseByte(((uint8_t*)buf)[i]);
      else
        return _onData((void *)((uint8_t*)buf+i), len-i);
    }
  } else if(_parseState == PARSE_REQ_BODY){
    if(_isMultipart){
      size_t i;
      for(i=0; i<len; i++){
        _parseMultipartPostByte(((uint8_t*)buf)[i], i == len - 1);
        _parsedLength++;
      }
    } else {
      if(_parsedLength == 0){
        if(_contentType.startsWith("application/x-www-form-urlencoded")){
          _isPlainPost = true;
        } else if(_contentType == "text/plain" && __is_param_char(((char*)buf)[0])){
          size_t i = 0;
          while (i<len && __is_param_char(((char*)buf)[i++]));
          if(i < len && ((char*)buf)[i-1] == '='){
            _isPlainPost = true;
          }
        }
      }
      if(!_isPlainPost) {
        if(_handler) _handler->handleBody(this, (uint8_t*)buf, len, _parsedLength, _contentLength);
        _parsedLength += len;
      } else {
        size_t i;
        for(i=0; i<len; i++){
          _parsedLength++;
          _parsePlainPostChar(((uint8_t*)buf)[i]);
        }
      }
    }

    if(_parsedLength == _contentLength){
      _parseState = PARSE_REQ_END;
      if(_handler) _handler->handleRequest(this);
      else send(501);
      return;
    }
  }
}

void AsyncWebServerRequest::_onPoll(){
  //os_printf("p\n");
  if(_response != NULL && _client != NULL && _client->canSend() && !_response->_finished()){
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
  if(p == NULL)
    return;
  if(_params == NULL)
    _params = p;
  else {
    AsyncWebParameter *ps = _params;
    while(ps->next != NULL) ps = ps->next;
    ps->next = p;
  }
}

void AsyncWebServerRequest::_addGetParam(String param){
  param = urlDecode(param);
  String name = param;
  String value = "";
  if(param.indexOf('=') > 0){
    name = param.substring(0, param.indexOf('='));
    value = param.substring(param.indexOf('=') + 1);
  }
  _addParam(new AsyncWebParameter(name, value));
}


bool AsyncWebServerRequest::_parseReqHead(){
  String m = _temp.substring(0, _temp.indexOf(' '));
  if(m == "GET"){
    _method = HTTP_GET;
  } else if(m == "POST"){
    _method = HTTP_POST;
  } else if(m == "DELETE"){
    _method = HTTP_DELETE;
  } else if(m == "PUT"){
    _method = HTTP_PUT;
  } else if(m == "PATCH"){
    _method = HTTP_PATCH;
  } else if(m == "HEAD"){
    _method = HTTP_HEAD;
  } else if(m == "OPTIONS"){
    _method = HTTP_OPTIONS;
  }

  _temp = _temp.substring(_temp.indexOf(' ')+1);
  String u = _temp.substring(0, _temp.indexOf(' '));
  u = urlDecode(u);
  String g = String();
  if(u.indexOf('?') > 0){
    g = u.substring(u.indexOf('?') + 1);
    u = u.substring(0, u.indexOf('?'));
  }
  _url = u;
  if(g.length()){
    while(true){
      if(g.length() == 0)
        break;
      if(g.indexOf('&') > 0){
        _addGetParam(g.substring(0, g.indexOf('&')));
        g = g.substring(g.indexOf('&') + 1);
      } else {
        _addGetParam(g);
        break;
      }
    }
  }

  _temp = _temp.substring(_temp.indexOf(' ')+1);
  if(_temp.startsWith("HTTP/1.1"))
    _version = 1;
  _temp = String();
  return true;
}

bool AsyncWebServerRequest::_parseReqHeader(){
  if(_temp.indexOf(':')){
    AsyncWebHeader *h = new AsyncWebHeader(_temp);
    if(h == NULL)
      return false;
    if(h->name() == "Host"){
      _host = h->value();
      delete h;
      _server->_handleRequest(this);
    } else if(h->name() == "Content-Type"){
      if (h->value().startsWith("multipart/")){
        _boundary = h->value().substring(h->value().indexOf('=')+1);
        _contentType = h->value().substring(0, h->value().indexOf(';'));
        _isMultipart = true;
      } else {
        _contentType = h->value();
      }
      delete h;
    } else if(h->name() == "Content-Length"){
      _contentLength = atoi(h->value().c_str());
      delete h;
    } else if(h->name() == "Expect" && h->value() == "100-continue"){
      _expectingContinue = true;
      delete h;
    } else if(h->name() == "Authorization"){
      if(h->value().startsWith("Basic")){
        _authorization = h->value().substring(6);
      }
      delete h;
    } else {
      if(_interestingHeaders->contains(h->name()) || _interestingHeaders->contains("ANY")){
        if(_headers == NULL)
          _headers = h;
        else {
          AsyncWebHeader *hs = _headers;
          while(hs->next != NULL) hs = hs->next;
          hs->next = h;
        }
      } else
        delete h;
    }

  }
  _temp = String();
  return true;
}

void AsyncWebServerRequest::_parsePlainPostChar(uint8_t data){
  if(data && (char)data != '&')
    _temp += (char)data;
  if(!data || (char)data == '&' || _parsedLength == _contentLength){
    _temp = urlDecode(_temp);
    String name = "body";
    String value = _temp;
    if(!_temp.startsWith("{") && !_temp.startsWith("[") && _temp.indexOf('=') > 0){
      name = _temp.substring(0, _temp.indexOf('='));
      value = _temp.substring(_temp.indexOf('=') + 1);
    }
    _addParam(new AsyncWebParameter(name, value, true));
    _temp = String();
  }
}

void AsyncWebServerRequest::_handleUploadByte(uint8_t data, bool last){
  _itemBuffer[_itemBufferIndex++] = data;

  if(last || _itemBufferIndex == 1460){
    if(_handler)
      _handler->handleUpload(this, _itemFilename, _itemSize - _itemBufferIndex, _itemBuffer, _itemBufferIndex, false);
    _itemBufferIndex = 0;
  }
}

enum {
  EXPECT_BOUNDARY,
  PARSE_HEADERS,
  WAIT_FOR_RETURN1,
  EXPECT_FEED1,
  EXPECT_DASH1,
  EXPECT_DASH2,
  BOUNDARY_OR_DATA,
  DASH3_OR_RETURN2,
  EXPECT_FEED2,
  PARSING_FINISHED,
  PARSE_ERROR
};

void AsyncWebServerRequest::_parseMultipartPostByte(uint8_t data, bool last){
#define itemWriteByte(b) do { _itemSize++; if(_itemIsFile) _handleUploadByte(b, last); else _itemValue+=(char)(b); } while(0)

  if(!_parsedLength){
    _multiParseState = EXPECT_BOUNDARY;
    _temp = String();
    _itemName = String();
    _itemFilename = String();
    _itemType = String();
  }

  if(_multiParseState == WAIT_FOR_RETURN1){
    if(data != '\r'){
      itemWriteByte(data);
    } else {
      _multiParseState = EXPECT_FEED1;
    }
  } else if(_multiParseState == EXPECT_BOUNDARY){
    if(_parsedLength < 2 && data != '-'){
      _multiParseState = PARSE_ERROR;
      return;
    } else if(_parsedLength - 2 < _boundary.length() && _boundary.c_str()[_parsedLength - 2] != data){
      _multiParseState = PARSE_ERROR;
      return;
    } else if(_parsedLength - 2 == _boundary.length() && data != '\r'){
      _multiParseState = PARSE_ERROR;
      return;
    } else if(_parsedLength - 3 == _boundary.length()){
      if(data != '\n'){
        _multiParseState = PARSE_ERROR;
        return;
      }
      _multiParseState = PARSE_HEADERS;
      _itemIsFile = false;
    }
  } else if(_multiParseState == PARSE_HEADERS){
    if((char)data != '\r' && (char)data != '\n')
       _temp += (char)data;
    if((char)data == '\n'){
      if(_temp.length()){
        if(_temp.startsWith("Content-Type:")){
          _itemType = _temp.substring(14);
          _itemIsFile = true;
        } else if(_temp.startsWith("Content-Disposition:")){
          _temp = _temp.substring(_temp.indexOf(';') + 2);
          while(_temp.indexOf(';') > 0){
            String name = _temp.substring(0, _temp.indexOf('='));
            String nameVal = _temp.substring(_temp.indexOf('=') + 2, _temp.indexOf(';') - 1);
            if(name == "name"){
              _itemName = nameVal;
            } else if(name == "filename"){
              _itemFilename = nameVal;
              _itemIsFile = true;
            }
            _temp = _temp.substring(_temp.indexOf(';') + 2);
          }
          String name = _temp.substring(0, _temp.indexOf('='));
          String nameVal = _temp.substring(_temp.indexOf('=') + 2, _temp.length() - 1);
          if(name == "name"){
            _itemName = nameVal;
          } else if(name == "filename"){
            _itemFilename = nameVal;
            _itemIsFile = true;
          }
        }
        _temp = String();
      } else {
        _multiParseState = WAIT_FOR_RETURN1;
        //value starts from here
        _itemSize = 0;
        _itemStartIndex = _parsedLength;
        _itemValue = String();
        if(_itemIsFile){
          if(_itemBuffer)
            free(_itemBuffer);
          _itemBuffer = (uint8_t*)malloc(1460);
          if(_itemBuffer == NULL){
            _multiParseState = PARSE_ERROR;
            return;
          }
          _itemBufferIndex = 0;
        }
      }
    }
  } else if(_multiParseState == EXPECT_FEED1){
    if(data != '\n'){
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r'); itemWriteByte(data);
    } else {
      _multiParseState = EXPECT_DASH1;
    }
  } else if(_multiParseState == EXPECT_DASH1){
    if(data != '-'){
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r'); itemWriteByte('\n');  itemWriteByte(data);
    } else {
      _multiParseState = EXPECT_DASH2;
    }
  } else if(_multiParseState == EXPECT_DASH2){
    if(data != '-'){
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r'); itemWriteByte('\n'); itemWriteByte('-');  itemWriteByte(data);
    } else {
      _multiParseState = BOUNDARY_OR_DATA;
      _boundaryPosition = 0;
    }
  } else if(_multiParseState == BOUNDARY_OR_DATA){
    if(_boundaryPosition < _boundary.length() && _boundary.c_str()[_boundaryPosition] != data){
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r'); itemWriteByte('\n'); itemWriteByte('-');  itemWriteByte('-');
      uint8_t i;
      for(i=0; i<_boundaryPosition; i++)
        itemWriteByte(_boundary.c_str()[i]);
      itemWriteByte(data);
    } else if(_boundaryPosition == _boundary.length() - 1){
      _multiParseState = DASH3_OR_RETURN2;
      if(!_itemIsFile){
        _addParam(new AsyncWebParameter(_itemName, _itemValue, true));
      } else {
        if(_itemSize){
          if(_handler) _handler->handleUpload(this, _itemFilename, _itemSize - _itemBufferIndex, _itemBuffer, _itemBufferIndex, true);
          _itemBufferIndex = 0;
          _addParam(new AsyncWebParameter(_itemName, _itemFilename, true, true, _itemSize));
        }
        free(_itemBuffer);
      }

    } else {
      _boundaryPosition++;
    }
  } else if(_multiParseState == DASH3_OR_RETURN2){
    if(data == '-' && (_contentLength - _parsedLength - 4) != 0){
      os_printf("ERROR: The parser got to the end of the POST but is expecting %u bytes more!\nDrop an issue so we can have more info on the matter!\n", _contentLength - _parsedLength - 4);
      _contentLength = _parsedLength + 4;//lets close the request gracefully
    }
    if(data == '\r'){
      _multiParseState = EXPECT_FEED2;
    } else if(data == '-' && _contentLength == (_parsedLength + 4)){
      _multiParseState = PARSING_FINISHED;
    } else {
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r'); itemWriteByte('\n'); itemWriteByte('-');  itemWriteByte('-');
      uint8_t i; for(i=0; i<_boundary.length(); i++) itemWriteByte(_boundary.c_str()[i]);
      itemWriteByte(data);
    }
  } else if(_multiParseState == EXPECT_FEED2){
    if(data == '\n'){
      _multiParseState = PARSE_HEADERS;
      _itemIsFile = false;
    } else {
      _multiParseState = WAIT_FOR_RETURN1;
      itemWriteByte('\r'); itemWriteByte('\n'); itemWriteByte('-');  itemWriteByte('-');
      uint8_t i; for(i=0; i<_boundary.length(); i++) itemWriteByte(_boundary.c_str()[i]);
      itemWriteByte('\r'); itemWriteByte(data);
    }
  }
}

void AsyncWebServerRequest::_parseLine(){
  if(_parseState == PARSE_REQ_START){
    if(!_temp.length()){
      _parseState = PARSE_REQ_FAIL;
      _client->close();
    } else {
      _parseReqHead();
      _parseState = PARSE_REQ_HEADERS;
    }
    return;
  }

  if(_parseState == PARSE_REQ_HEADERS){
    if(!_temp.length()){
      //end of headers
      if(_expectingContinue){
        const char * response = "HTTP/1.1 100 Continue\r\n\r\n";
        _client->write(response, os_strlen(response));
      }
      if(_contentLength){
        _parseState = PARSE_REQ_BODY;
      } else {
        _parseState = PARSE_REQ_END;
        if(_handler) _handler->handleRequest(this);
        else send(501);
      }
    } else _parseReqHeader();
  }
}

void AsyncWebServerRequest::_parseByte(uint8_t data){
  if((char)data != '\r' && (char)data != '\n')
    _temp += (char)data;
  if((char)data == '\n')
    _parseLine();
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
  if(_response == NULL){
    _client->close(true);
    _onDisconnect();
    return;
  }
  if(!_response->_sourceValid()){
    delete response;
    _response = NULL;
    send(500);
  }
  else
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
      char *toencode = new char[toencodeLen+1];
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
