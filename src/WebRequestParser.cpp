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
#include <WebResponseImpl.h>
#include "ESPAsyncWebServer.h"

#ifndef ESP8266
#define os_strlen strlen
#endif


#define __is_param_char(c) ((c) && ((c) != '{') && ((c) != '[') && ((c) != '&') && ((c) != '='))

enum { PARSE_REQ_START, PARSE_REQ_HEADERS, PARSE_REQ_BODY, PARSE_REQ_END, PARSE_REQ_FAIL };

void AsyncWebServerRequest::_parseByte(uint8_t data){
  if((char)data != '\r' && (char)data != '\n')
    _temp += (char)data;
  if((char)data == '\n')
    _parseLine();
}

void AsyncWebServerRequest::_parseLine(){
  if(_parseState == PARSE_REQ_START){
    if(_temp.length()){
      _parseReqHead();//head!
      _parseState = PARSE_REQ_HEADERS;
    } else {
      _parseState = PARSE_REQ_FAIL;
      _client->close();
    }
    return;
  }

  if(_parseState == PARSE_REQ_HEADERS){
    if(!_temp.length()){
      _parseReqHeader();//header!
    } else {
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
    }
  }
}

void AsyncWebServerRequest::_onData(void *buf, size_t len){
  if(_parseState < PARSE_REQ_BODY){
    //ToDo: make it parse the whole packet at once
    size_t i;
    for(i=0; i<len; i++){
      if(_parseState < PARSE_REQ_BODY)
        _parseByte(((uint8_t*)buf)[i]);//parse byte for head/header
      else
        return _onData((void *)((uint8_t*)buf+i), len-i);//the last byte switched the mode so forward
    }

  } else if(_parseState == PARSE_REQ_BODY){
    if(_isMultipart){
      size_t i;
      for(i=0; i<len; i++){
        _parseMultipartPostByte(((uint8_t*)buf)[i], i == len - 1);//parse multipart post byte
        _parsedLength++;
      }

    } else {
      //ToDo: make it parse the whole packet at once
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
        //ToDo: make it parse the whole packet at once
        size_t i;
        for(i=0; i<len; i++){
          _parsedLength++;
          _parsePlainPostChar(((uint8_t*)buf)[i]);//parse plain post byte
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

//
//  REQUEST HEAD
//

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
  //Method
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

  //URL
  _temp = _temp.substring(_temp.indexOf(' ')+1);
  String u = _temp.substring(0, _temp.indexOf(' '));
  String g = String();
  if(u.indexOf('?') > 0){
    g = u.substring(u.indexOf('?') + 1);
    u = u.substring(0, u.indexOf('?'));
  }
  _url = u;

  //Parameters
  if(g.length()){
    while(true){
      if(g.length() == 0)
        break;
      if(g.indexOf('&') > 0){
        _addGetParam(g.substring(0, g.indexOf('&')));//GET Param
        g = g.substring(g.indexOf('&') + 1);
      } else {
        _addGetParam(g);//Get Param
        break;
      }
    }
  }

  //Version
  _temp = _temp.substring(_temp.indexOf(' ')+1);
  if(_temp.startsWith("HTTP/1.1"))
    _version = 1;
  _temp = String();
  return true;
}

//
//  HEADERS
//

bool AsyncWebServerRequest::_parseReqHeader(){
  if(_temp.indexOf(':')){
    AsyncWebHeader *h = new AsyncWebHeader(_temp);
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

//
//  PLAIN POST
//

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

//
//  MULTIPART POST
//

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

void AsyncWebServerRequest::_handleUploadByte(uint8_t data, bool last){
  _itemBuffer[_itemBufferIndex++] = data;
  if(last){
    if(_handler)
      _handler->handleUpload(this, _itemFilename, _itemSize - _itemBufferIndex, _itemBuffer, _itemBufferIndex, false);
    _itemBufferIndex = 0;
  }
}

void AsyncWebServerRequest::_parseMultipartPostByte(uint8_t data, bool last){
#define itemWriteByte(b) do { _itemSize++; if(_itemIsFile) _handleUploadByte(b, last); else _itemValue+=(char)(b); } while(0)

  if(!_parsedLength){
    _multiParseState = EXPECT_BOUNDARY;
    _temp = String();
    _itemName = String();
    _itemFilename = String();
    _itemType = String();
  }

  if(_multiParseState == EXPECT_BOUNDARY){
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
          _itemBufferIndex = 0;
        }
      }
    }
  } else if(_multiParseState == WAIT_FOR_RETURN1){
    if(data != '\r'){
      itemWriteByte(data);
    } else {
      _multiParseState = EXPECT_FEED1;
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





