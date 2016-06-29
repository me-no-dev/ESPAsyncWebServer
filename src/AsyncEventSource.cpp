/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.

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
#include "Arduino.h"
#include "AsyncEventSource.h"

static String generateEventMessage(const char *message, const char *event, uint32_t id, uint32_t reconnect){
  String ev = "";

  if(reconnect){
    ev += "retry: ";
    ev += String(reconnect);
    ev += "\r\n";
  }

  if(id){
    ev += "id: ";
    ev += String(reconnect);
    ev += "\r\n";
  }

  if(event != NULL){
    ev += "event: ";
    ev += String(event);
    ev += "\r\n";
  }

  if(message != NULL){
    size_t messageLen = strlen(message);
    char * lineStart = (char *)message;
    char * lineEnd;
    do {
      char * nextN = strchr(lineStart, '\n');
      char * nextR = strchr(lineStart, '\r');
      if(nextN == NULL && nextR == NULL){
        size_t llen = ((char *)message + messageLen) - lineStart;
        char * ldata = (char *)malloc(llen+1);
        if(ldata != NULL){
          memcpy(ldata, lineStart, llen);
          ldata[llen] = 0;
          ev += "data: ";
          ev += ldata;
          ev += "\r\n\r\n";
          free(ldata);
        }
        lineStart = (char *)message + messageLen;
      } else {
        char * nextLine = NULL;
        if(nextN != NULL && nextR != NULL){
          if(nextR < nextN){
            lineEnd = nextR;
            if(nextN == (nextR + 1))
              nextLine = nextN + 1;
            else
              nextLine = nextR + 1;
          } else {
            lineEnd = nextN;
            if(nextR == (nextN + 1))
              nextLine = nextR + 1;
            else
              nextLine = nextN + 1;
          }
        } else if(nextN != NULL){
          lineEnd = nextN;
          nextLine = nextN + 1;
        } else {
          lineEnd = nextR;
          nextLine = nextR + 1;
        }

        size_t llen = lineEnd - lineStart;
        char * ldata = (char *)malloc(llen+1);
        if(ldata != NULL){
          memcpy(ldata, lineStart, llen);
          ldata[llen] = 0;
          ev += "data: ";
          ev += ldata;
          ev += "\r\n";
          free(ldata);
        }
        lineStart = nextLine;
        if(lineStart == ((char *)message + messageLen))
          ev += "\r\n";
      }
    } while(lineStart < ((char *)message + messageLen));
  }

  return ev;
}

// Client

AsyncEventSourceClient::AsyncEventSourceClient(AsyncWebServerRequest *request, AsyncEventSource *server){
  _client = request->client();
  _server = server;
  next = NULL;
  //_client->onError([](void *r, AsyncClient* c, int8_t error){ ((AsyncEventSourceClient*)(r))->_onError(error); }, this);
  //_client->onAck([](void *r, AsyncClient* c, size_t len, uint32_t time){ ((AsyncEventSourceClient*)(r))->_onAck(len, time); }, this);
  //_client->onPoll([](void *r, AsyncClient* c){ ((AsyncEventSourceClient*)(r))->_onPoll(); }, this);
  _client->onError(NULL, NULL);
  _client->onAck(NULL, NULL);
  _client->onPoll(NULL, NULL);
  _client->onData(NULL, NULL);
  _client->onTimeout([](void *r, AsyncClient* c, uint32_t time){ ((AsyncEventSourceClient*)(r))->_onTimeout(time); }, this);
  _client->onDisconnect([](void *r, AsyncClient* c){ ((AsyncEventSourceClient*)(r))->_onDisconnect(); }, this);
  _server->_addClient(this);
  delete request;
}

AsyncEventSourceClient::~AsyncEventSourceClient(){
  close();
}

//void AsyncEventSourceClient::_onAck(size_t len, uint32_t time){}
//void AsyncEventSourceClient::_onPoll(){}
//void AsyncEventSourceClient::_onError(int8_t){}
void AsyncEventSourceClient::_onTimeout(uint32_t time){
  _client->close(true);
}

void AsyncEventSourceClient::_onDisconnect(){
  AsyncClient* cl = _client;
  _client = NULL;
  cl->free();
  delete cl;
  _server->_handleDisconnect(this);
}

void AsyncEventSourceClient::close(){
  if(_client != NULL)
    _client->close(true);
}

void AsyncEventSourceClient::write(const char * message, size_t len){
  if(!_client->canSend()){
    return;
  }
  if(_client->space() < len){
    return;
  }
  _client->write(message, len);
}

void AsyncEventSourceClient::send(const char *message, const char *event, uint32_t id, uint32_t reconnect){
  String ev = generateEventMessage(message, event, id, reconnect);
  write(ev.c_str(), ev.length());
}


// Handler

AsyncEventSource::AsyncEventSource(String url)
  : _url(url)
  , _clients(NULL)
  , _connectcb(NULL)
{}

AsyncEventSource::~AsyncEventSource(){
  close();
}

void AsyncEventSource::onConnect(ArEventHandlerFunction cb){
  _connectcb = cb;
}

void AsyncEventSource::_addClient(AsyncEventSourceClient * client){
  if(_clients == NULL){
    _clients = client;
    if(_connectcb)
      _connectcb(client);
    return;
  }
  AsyncEventSourceClient * c = _clients;
  while(c->next != NULL) c = c->next;
  c->next = client;
  if(_connectcb)
    _connectcb(client);
}

void AsyncEventSource::_handleDisconnect(AsyncEventSourceClient * client){
  if(_clients == NULL){
    return;
  }
  if(_clients == client){
    _clients = client->next;
    delete client;
    return;
  }
  AsyncEventSourceClient * c = _clients;
  while(c->next != NULL && c->next != client) c = c->next;
  if(c->next == NULL){
    return;
  }
  c->next = client->next;
  delete client;
}

void AsyncEventSource::close(){
  AsyncEventSourceClient * c = _clients;
  while(c != NULL){
    if(c->connected())
      c->close();
    c = c->next;
  }
}

void AsyncEventSource::send(const char *message, const char *event, uint32_t id, uint32_t reconnect){
  if(_clients == NULL)
    return;

  String ev = generateEventMessage(message, event, id, reconnect);
  AsyncEventSourceClient * c = _clients;
  while(c != NULL){
    if(c->connected())
      c->write(ev.c_str(), ev.length());
    c = c->next;
  }
}

bool AsyncEventSource::canHandle(AsyncWebServerRequest *request){
  if(request->method() != HTTP_GET || !request->url().equals(_url))
    return false;
  return true;
}

void AsyncEventSource::handleRequest(AsyncWebServerRequest *request){
  request->send(new AsyncEventSourceResponse(this));
}

// Response

AsyncEventSourceResponse::AsyncEventSourceResponse(AsyncEventSource *server){
  _server = server;
  _code = 200;
  _contentType = "text/event-stream";
  _sendContentLength = false;
  addHeader("Cache-Control", "no-cache");
  addHeader("Connection","keep-alive");
}

void AsyncEventSourceResponse::_respond(AsyncWebServerRequest *request){
  String out = _assembleHead(request->version());
  request->client()->write(out.c_str(), _headLength);
  _state = RESPONSE_WAIT_ACK;
}

size_t AsyncEventSourceResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time){
  if(len){
    new AsyncEventSourceClient(request, _server);
  }
  return 0;
}
