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
#include "Arduino.h"
#include "AsyncWebSocket.h"

#include <libb64/cencode.h>

#ifndef ESP8266
extern "C" {
typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Transform(uint32_t state[5], const unsigned char buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, const unsigned char* data, uint32_t len);
void SHA1Final(unsigned char digest[20], SHA1_CTX* context);
}
#else
#include <Hash.h>
#endif


size_t webSocketSendFrameWindow(AsyncClient *client){
  if(!client->canSend())
    return 0;
  size_t space = client->space();
  if(space < 9)
    return 0;
  return space - 8;
}

size_t webSocketSendFrame(AsyncClient *client, bool final, uint8_t opcode, bool mask, uint8_t *data, size_t len){
  if(!client->canSend())
    return 0;
  size_t space = client->space();
  if(space < 2)
    return 0;
  uint8_t mbuf[4] = {0,0,0,0};
  uint8_t headLen = 2;
  if(len && mask){
    headLen += 4;
    mbuf[0] = rand() % 0xFF;
    mbuf[1] = rand() % 0xFF;
    mbuf[2] = rand() % 0xFF;
    mbuf[3] = rand() % 0xFF;
  }
  if(len > 125)
    headLen += 2;
  if(space < headLen)
    return 0;
  space -= headLen;

  if(len > space) len = space;

  uint8_t *buf = (uint8_t*)malloc(headLen);
  if(buf == NULL){
    //os_printf("could not malloc %u bytes for frame header\n", headLen);
    return 0;
  }

  buf[0] = opcode & 0x0F;
  if(final)
    buf[0] |= 0x80;
  if(len < 126)
    buf[1] = len & 0x7F;
  else {
    buf[1] = 126;
    buf[2] = (uint8_t)((len >> 8) & 0xFF);
    buf[3] = (uint8_t)(len & 0xFF);
  }
  if(len && mask){
    buf[1] |= 0x80;
    memcpy(buf + (headLen - 4), mbuf, 4);
  }
  if(client->add((const char *)buf, headLen) != headLen){
    //os_printf("error adding %lu header bytes\n", headLen);
    free(buf);
    return 0;
  }
  free(buf);

  if(len){
    if(len && mask){
      size_t i;
      for(i=0;i<len;i++)
        data[i] = data[i] ^ mbuf[i%4];
    }
    if(client->add((const char *)data, len) != len){
      //os_printf("error adding %lu data bytes\n", len);
      return 0;
    }
  }
  if(!client->send()){
    //os_printf("error sending frame: %lu\n", headLen+len);
    return 0;
  }
  return len;
}




/*
 * Control Frame
 */

class AsyncWebSocketControl {
  private:
    uint8_t _opcode;
    uint8_t *_data;
    size_t _len;
    bool _mask;
    bool _finished;
  public:
    AsyncWebSocketControl * next;
    AsyncWebSocketControl(uint8_t opcode, uint8_t *data=NULL, size_t len=0, bool mask=false)
      :_opcode(opcode)
      ,_len(len)
      ,_mask(len && mask)
      ,_finished(false)
      ,next(NULL){
      if(data == NULL)
        _len = 0;
      if(_len){
        if(_len > 125)
          _len = 125;
        _data = (uint8_t*)malloc(_len);
        if(_data == NULL)
          _len = 0;
        else memcpy(_data, data, len);
      } else _data = NULL;
    }
    ~AsyncWebSocketControl(){
      if(_data != NULL)
        free(_data);
    }
    bool finished(){ return _finished; }
    uint8_t opcode(){ return _opcode; }
    uint8_t len(){ return _len + 2; }
    size_t send(AsyncClient *client){
      _finished = true;
      return webSocketSendFrame(client, true, _opcode & 0x0F, _mask, _data, _len);
    }
};

/*
 * Basic Buffered Message
 */

class AsyncWebSocketBasicMessage: public AsyncWebSocketMessage {
  private:
    uint8_t * _data;
    size_t _len;
    size_t _sent;
    size_t _ack;
    size_t _acked;

  public:
    AsyncWebSocketBasicMessage(const char * data, size_t len, uint8_t opcode=WS_TEXT, bool mask=false)
      :_len(len)
      ,_sent(0)
      ,_ack(0)
      ,_acked(0)
    {
      _opcode = opcode & 0x07;
      _mask = mask;
      _data = (uint8_t*)malloc(_len+1);
      if(_data == NULL){
        _len = 0;
        _status = WS_MSG_ERROR;
      } else {
        _status = WS_MSG_SENDING;
        memcpy(_data, data, _len);
        _data[_len] = 0;
      }
    }
    virtual ~AsyncWebSocketBasicMessage(){
      if(_data != NULL)
        free(_data);
    }
    virtual bool betweenFrames(){ return _acked == _ack; }
    virtual void ack(size_t len, uint32_t time){
      _acked += len;
      if(_sent == _len && _acked == _ack){
        _status = WS_MSG_SENT;
      }
    }
    virtual size_t send(AsyncClient *client){
      if(_status != WS_MSG_SENDING)
        return 0;
      if(_acked < _ack){
        return 0;
      }
      if(_sent == _len){
        if(_acked == _ack)
          _status = WS_MSG_SENT;
        return 0;
      }
      size_t window = webSocketSendFrameWindow(client);
      size_t toSend = _len - _sent;
      if(window < toSend) toSend = window;
      bool final = ((toSend + _sent) == _len);
      size_t sent = webSocketSendFrame(client, final, (_sent == 0)?_opcode:WS_CONTINUATION, _mask, (uint8_t*)(_data+_sent), toSend);
      _sent += sent;
      uint8_t headLen = ((sent < 126)?2:4)+(_mask*4);
      _ack += sent + headLen;
      return sent;
    }
};





/*
 * Async WebSocket Client
 */
 const char * AWSC_PING_PAYLOAD = "ESPAsyncWebServer-PING";
 const size_t AWSC_PING_PAYLOAD_LEN = 22;

AsyncWebSocketClient::AsyncWebSocketClient(AsyncWebServerRequest *request, AsyncWebSocket *server){
  _client = request->client();
  _server = server;
  _clientId = _server->_getNextId();
  _status = WS_CONNECTED;
  _controlQueue = NULL;
  _messageQueue = NULL;
  _pstate = 0;
  _lastMessageTime = millis();
  _keepAlivePeriod = 0;
  next = NULL;
  _client->onError([](void *r, AsyncClient* c, int8_t error){ ((AsyncWebSocketClient*)(r))->_onError(error); }, this);
  _client->onAck([](void *r, AsyncClient* c, size_t len, uint32_t time){ ((AsyncWebSocketClient*)(r))->_onAck(len, time); }, this);
  _client->onDisconnect([](void *r, AsyncClient* c){ ((AsyncWebSocketClient*)(r))->_onDisconnect(); delete c; }, this);
  _client->onTimeout([](void *r, AsyncClient* c, uint32_t time){ ((AsyncWebSocketClient*)(r))->_onTimeout(time); }, this);
  _client->onData([](void *r, AsyncClient* c, void *buf, size_t len){ ((AsyncWebSocketClient*)(r))->_onData(buf, len); }, this);
  _client->onPoll([](void *r, AsyncClient* c){ ((AsyncWebSocketClient*)(r))->_onPoll(); }, this);
  _server->_addClient(this);
  _server->_handleEvent(this, WS_EVT_CONNECT, NULL, NULL, 0);
  delete request;
}

AsyncWebSocketClient::~AsyncWebSocketClient(){
  while(_messageQueue != NULL){
    AsyncWebSocketMessage * m = _messageQueue;
    _messageQueue = _messageQueue->next;
    delete(m);
  }
  while(_controlQueue != NULL){
    AsyncWebSocketControl * c = _controlQueue;
    _controlQueue = _controlQueue->next;
    delete(c);
  }
  _server->_handleEvent(this, WS_EVT_DISCONNECT, NULL, NULL, 0);
}

void AsyncWebSocketClient::_onAck(size_t len, uint32_t time){
  _lastMessageTime = millis();
  if(_controlQueue != NULL){
    AsyncWebSocketControl *controlMessage = _controlQueue;
    if(controlMessage->finished()){
      _controlQueue = _controlQueue->next;
      len -= controlMessage->len();
      if(_status == WS_DISCONNECTING && controlMessage->opcode() == WS_DISCONNECT){
        delete controlMessage;
        _status = WS_DISCONNECTED;
        _client->close(true);
        return;
      }
      delete controlMessage;
    }
  }
  if(len && _messageQueue != NULL){
    _messageQueue->ack(len, time);
  }
  _runQueue();
}

void AsyncWebSocketClient::_onPoll(){
  if(_client->canSend() && (_controlQueue != NULL || _messageQueue != NULL)){
    _runQueue();
  } else if(_keepAlivePeriod > 0 && _controlQueue == NULL && _messageQueue == NULL && (millis() - _lastMessageTime) >= _keepAlivePeriod){
    ping((uint8_t *)AWSC_PING_PAYLOAD, AWSC_PING_PAYLOAD_LEN);
  }
}

void AsyncWebSocketClient::_runQueue(){
  while(_messageQueue != NULL && _messageQueue->finished()){
    AsyncWebSocketMessage * m = _messageQueue;
    _messageQueue = _messageQueue->next;
    delete(m);
  }

  if(_controlQueue != NULL && (_messageQueue == NULL || _messageQueue->betweenFrames()) && webSocketSendFrameWindow(_client) > (size_t)(_controlQueue->len() - 1)){
    AsyncWebSocketControl *control = _controlQueue;
    control->send(_client);
  } else if(_messageQueue != NULL && _messageQueue->betweenFrames() && webSocketSendFrameWindow(_client)){
    _messageQueue->send(_client);
  }
}

void AsyncWebSocketClient::_queueMessage(AsyncWebSocketMessage *dataMessage){
  if(dataMessage == NULL)
    return;
  if(_status != WS_CONNECTED){
    delete dataMessage;
    return;
  }
  if(_messageQueue == NULL){
    _messageQueue = dataMessage;
  } else {
    AsyncWebSocketMessage * m = _messageQueue;
    while(m->next != NULL) m = m->next;
    m->next = dataMessage;
  }
  if(_client->canSend())
    _runQueue();
}

void AsyncWebSocketClient::_queueControl(AsyncWebSocketControl *controlMessage){
  if(controlMessage == NULL)
    return;
  if(_controlQueue == NULL){
    _controlQueue = controlMessage;
  } else {
    AsyncWebSocketControl * m = _controlQueue;
    while(m->next != NULL) m = m->next;
    m->next = controlMessage;
  }
  if(_client->canSend())
    _runQueue();
}

void AsyncWebSocketClient::close(uint16_t code, const char * message){
  if(_status != WS_CONNECTED)
    return;
  if(code){
    uint8_t packetLen = 2;
    if(message != NULL){
      size_t mlen = strlen(message);
      if(mlen > 123) mlen = 123;
      packetLen += mlen;
    }
    char * buf = (char*)malloc(packetLen);
    if(buf != NULL){
      buf[0] = (uint8_t)(code >> 8);
      buf[1] = (uint8_t)(code & 0xFF);
      if(message != NULL){
        memcpy(buf+2, message, packetLen -2);
      }
      _queueControl(new AsyncWebSocketControl(WS_DISCONNECT,(uint8_t*)buf,packetLen));
      free(buf);
      return;
    }
  }
  _queueControl(new AsyncWebSocketControl(WS_DISCONNECT));
}

void AsyncWebSocketClient::ping(uint8_t *data, size_t len){
  if(_status == WS_CONNECTED)
    _queueControl(new AsyncWebSocketControl(WS_PING, data, len));
}

void AsyncWebSocketClient::_onError(int8_t){}

void AsyncWebSocketClient::_onTimeout(uint32_t time){
  _client->close(true);
}

void AsyncWebSocketClient::_onDisconnect(){
  _client = NULL;
  _server->_handleDisconnect(this);
}

void AsyncWebSocketClient::_onData(void *buf, size_t plen){
  _lastMessageTime = millis();
  uint8_t *fdata = (uint8_t*)buf;
  uint8_t * data = fdata;
  if(!_pstate){
    _pinfo.index = 0;
    _pinfo.final = (fdata[0] & 0x80) != 0;
    _pinfo.opcode = fdata[0] & 0x0F;
    _pinfo.masked = (fdata[1] & 0x80) != 0;
    _pinfo.len = fdata[1] & 0x7F;
    data += 2;
    plen = plen - 2;
    if(_pinfo.len == 126){
      _pinfo.len = fdata[3] | (uint16_t)(fdata[2]) << 8;
      data += 2;
      plen = plen - 2;
    } else if(_pinfo.len == 127){
      _pinfo.len = fdata[9] | (uint16_t)(fdata[8]) << 8 | (uint32_t)(fdata[7]) << 16 | (uint32_t)(fdata[6]) << 24 | (uint64_t)(fdata[5]) << 32 | (uint64_t)(fdata[4]) << 40 | (uint64_t)(fdata[3]) << 48 | (uint64_t)(fdata[2]) << 56;
      data += 8;
      plen = plen - 8;
    }

    if(_pinfo.masked){
      memcpy(_pinfo.mask, data, 4);
      data += 4;
      plen = plen - 4;
      size_t i;
      for(i=0;i<plen;i++)
        data[i] = data[i] ^ _pinfo.mask[(_pinfo.index+i)%4];
    }
  } else {
    if(_pinfo.masked){
      size_t i;
      for(i=0;i<plen;i++)
        data[i] = data[i] ^ _pinfo.mask[(_pinfo.index+i)%4];
    }
  }
  if((plen + _pinfo.index) < _pinfo.len){
    _pstate = 1;

    if(_pinfo.index == 0){
      if(_pinfo.opcode){
        _pinfo.message_opcode = _pinfo.opcode;
        _pinfo.num = 0;
      } else _pinfo.num += 1;
    }
    _server->_handleEvent(this, WS_EVT_DATA, (void *)&_pinfo, (uint8_t*)data, plen);

    _pinfo.index += plen;
  } else if((plen + _pinfo.index) == _pinfo.len){
    _pstate = 0;
    if(_pinfo.opcode == WS_DISCONNECT){
      if(plen){
        uint16_t reasonCode = (uint16_t)(data[0] << 8) + data[1];
        char * reasonString = (char*)(data+2);
        if(reasonCode > 1001){
          _server->_handleEvent(this, WS_EVT_ERROR, (void *)&reasonCode, (uint8_t*)reasonString, strlen(reasonString));
        }
      }
      if(_status == WS_DISCONNECTING){
        _status = WS_DISCONNECTED;
        _client->close(true);
      } else {
        _status = WS_DISCONNECTING;
        _queueControl(new AsyncWebSocketControl(WS_DISCONNECT, data, plen));
      }
    } else if(_pinfo.opcode == WS_PING){
      _queueControl(new AsyncWebSocketControl(WS_PONG, data, plen));
    } else if(_pinfo.opcode == WS_PONG){
      if(plen != AWSC_PING_PAYLOAD_LEN || memcmp(AWSC_PING_PAYLOAD, data, AWSC_PING_PAYLOAD_LEN) != 0)
        _server->_handleEvent(this, WS_EVT_PONG, NULL, (uint8_t*)data, plen);
    } else if(_pinfo.opcode < 8){//continuation or text/binary frame
      _server->_handleEvent(this, WS_EVT_DATA, (void *)&_pinfo, (uint8_t*)data, plen);
    }
  } else {
    //os_printf("frame error: len: %u, index: %llu, total: %llu\n", plen, _pinfo.index, _pinfo.len);
    //what should we do?
  }
}

size_t AsyncWebSocketClient::printf(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char* temp = new char[64];
  if(!temp){
    return 0;
  }
  char* buffer = temp;
  size_t len = vsnprintf(temp, 64, format, arg);
  va_end(arg);
  if (len > 63) {
    buffer = new char[len + 1];
    if (!buffer) {
      return 0;
    }
    va_start(arg, format);
    vsnprintf(buffer, len + 1, format, arg);
    va_end(arg);
  }
  text(buffer, len);
  if (buffer != temp) {
    delete[] buffer;
  }
  delete[] temp;
  return len;
}

size_t AsyncWebSocketClient::printf_P(PGM_P formatP, ...) {
  va_list arg;
  va_start(arg, formatP);
  char* temp = new char[64];
  if(!temp){
    return 0;
  }
  char* buffer = temp;
  size_t len = vsnprintf_P(temp, 64, formatP, arg);
  va_end(arg);
  if (len > 63) {
    buffer = new char[len + 1];
    if (!buffer) {
      return 0;
    }
    va_start(arg, formatP);
    vsnprintf_P(buffer, len + 1, formatP, arg);
    va_end(arg);
  }
  text(buffer, len);
  if (buffer != temp) {
    delete[] buffer;
  }
  delete[] temp;
  return len;
}

void AsyncWebSocketClient::text(const char * message, size_t len){
  _queueMessage(new AsyncWebSocketBasicMessage(message, len));
}
void AsyncWebSocketClient::text(const char * message){
  text(message, strlen(message));
}
void AsyncWebSocketClient::text(uint8_t * message, size_t len){
  text((const char *)message, len);
}
void AsyncWebSocketClient::text(char * message){
  text(message, strlen(message));
}
void AsyncWebSocketClient::text(const String &message){
  text(message.c_str(), message.length());
}
void AsyncWebSocketClient::text(const __FlashStringHelper *data){
  PGM_P p = reinterpret_cast<PGM_P>(data);
  size_t n = 0;
  while (1) {
    if (pgm_read_byte(p+n) == 0) break;
      n += 1;
  }
  char * message = (char*) malloc(n+1);
  if(message){
    for(size_t b=0; b<n; b++)
      message[b] = pgm_read_byte(p++);
    message[n] = 0;
    text(message, n);
  }
}

void AsyncWebSocketClient::binary(const char * message, size_t len){
  _queueMessage(new AsyncWebSocketBasicMessage(message, len, WS_BINARY));
}
void AsyncWebSocketClient::binary(const char * message){
  binary(message, strlen(message));
}
void AsyncWebSocketClient::binary(uint8_t * message, size_t len){
  binary((const char *)message, len);
}
void AsyncWebSocketClient::binary(char * message){
  binary(message, strlen(message));
}
void AsyncWebSocketClient::binary(const String &message){
  binary(message.c_str(), message.length());
}
void AsyncWebSocketClient::binary(const __FlashStringHelper *data, size_t len){
  PGM_P p = reinterpret_cast<PGM_P>(data);
  char * message = (char*) malloc(len);
  if(message){
    for(size_t b=0; b<len; b++)
      message[b] = pgm_read_byte(p++);
    binary(message, len);
  }
}

IPAddress AsyncWebSocketClient::remoteIP() {
    if(!_client) {
        return IPAddress(0U);
    }
    return _client->remoteIP();
}

uint16_t AsyncWebSocketClient::remotePort() {
    if(!_client) {
        return 0;
    }
    return _client->remotePort();
}



/*
 * Async Web Socket - Each separate socket location
 */

AsyncWebSocket::AsyncWebSocket(String url)
  :_url(url)
  ,_clients(NULL)
  ,_cNextId(1)
  ,_enabled(true)
{
  _eventHandler = NULL;
}

AsyncWebSocket::~AsyncWebSocket(){}

void AsyncWebSocket::_handleEvent(AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(_eventHandler != NULL){
    _eventHandler(this, client, type, arg, data, len);
  }
}

void AsyncWebSocket::_addClient(AsyncWebSocketClient * client){
  if(_clients == NULL){
    _clients = client;
    return;
  }
  AsyncWebSocketClient * c = _clients;
  while(c->next != NULL) c = c->next;
  c->next = client;
}

void AsyncWebSocket::_handleDisconnect(AsyncWebSocketClient * client){
  if(_clients == NULL){
    return;
  }
  if(_clients->id() == client->id()){
    _clients = client->next;
    delete client;
    return;
  }
  AsyncWebSocketClient * c = _clients;
  while(c->next != NULL && c->next->id() != client->id()) c = c->next;
  if(c->next == NULL){
    return;
  }
  c->next = client->next;
  delete client;
}

size_t AsyncWebSocket::count(){
  size_t i = 0;
  AsyncWebSocketClient * c = _clients;
  while(c != NULL){
    if(c->status() == WS_CONNECTED)
      i++;
    c = c->next;
  }
  return i;
}

AsyncWebSocketClient * AsyncWebSocket::client(uint32_t id){
  AsyncWebSocketClient * c = _clients;
  while(c != NULL && c->id() != id)
    c = c->next;
  if(c != NULL && c->status() == WS_CONNECTED)
    return c;
  return NULL;
}


void AsyncWebSocket::close(uint32_t id, uint16_t code, const char * message){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL)
    c->close(code, message);
}

void AsyncWebSocket::closeAll(uint16_t code, const char * message){
  AsyncWebSocketClient * c = _clients;
  while(c != NULL){
    if(c->status() == WS_CONNECTED)
      c->close(code, message);
    c = c->next;
  }
}

void AsyncWebSocket::ping(uint32_t id, uint8_t *data, size_t len){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL)
    c->ping(data, len);
}

void AsyncWebSocket::pingAll(uint8_t *data, size_t len){
  AsyncWebSocketClient * c = _clients;
  while(c != NULL){
    if(c->status() == WS_CONNECTED)
      c->ping(data, len);
    c = c->next;
  }

}

void AsyncWebSocket::text(uint32_t id, const char * message, size_t len){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL)
    c->text(message, len);
}

void AsyncWebSocket::textAll(const char * message, size_t len){
  AsyncWebSocketClient * c = _clients;
  while(c != NULL){
    if(c->status() == WS_CONNECTED)
      c->text(message, len);
    c = c->next;
  }
}

void AsyncWebSocket::binary(uint32_t id, const char * message, size_t len){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL)
    c->binary(message, len);
}

void AsyncWebSocket::binaryAll(const char * message, size_t len){
  AsyncWebSocketClient * c = _clients;
  while(c != NULL){
    if(c->status() == WS_CONNECTED)
      c->binary(message, len);
    c = c->next;
  }
}

void AsyncWebSocket::message(uint32_t id, AsyncWebSocketMessage *message){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL)
    c->message(message);
}

void AsyncWebSocket::messageAll(AsyncWebSocketMessage *message){
  AsyncWebSocketClient * c = _clients;
  while(c != NULL){
    if(c->status() == WS_CONNECTED)
      c->message(message);
    c = c->next;
  }
}

size_t AsyncWebSocket::printf(uint32_t id, const char *format, ...){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL){
    va_list arg;
    va_start(arg, format);
    size_t len = c->printf(format, arg);
    va_end(arg);
    return len;
  }
  return 0;
}

size_t AsyncWebSocket::printfAll(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char* temp = new char[64];
  if(!temp){
    return 0;
  }
  char* buffer = temp;
  size_t len = vsnprintf(temp, 64, format, arg);
  va_end(arg);
  if (len > 63) {
    buffer = new char[len + 1];
    if (!buffer) {
      return 0;
    }
    va_start(arg, format);
    vsnprintf(buffer, len + 1, format, arg);
    va_end(arg);
  }
  textAll(buffer, len);
  if (buffer != temp) {
    delete[] buffer;
  }
  delete[] temp;
  return len;
}

size_t AsyncWebSocket::printf_P(uint32_t id, PGM_P formatP, ...){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL){
    va_list arg;
    va_start(arg, formatP);
    size_t len = c->printf_P(formatP, arg);
    va_end(arg);
    return len;
  }
  return 0;
}

size_t AsyncWebSocket::printfAll_P(PGM_P formatP, ...) {
  va_list arg;
  va_start(arg, formatP);
  char* temp = new char[64];
  if(!temp){
    return 0;
  }
  char* buffer = temp;
  size_t len = vsnprintf_P(temp, 64, formatP, arg);
  va_end(arg);
  if (len > 63) {
    buffer = new char[len + 1];
    if (!buffer) {
      return 0;
    }
    va_start(arg, formatP);
    vsnprintf_P(buffer, len + 1, formatP, arg);
    va_end(arg);
  }
  textAll(buffer, len);
  if (buffer != temp) {
    delete[] buffer;
  }
  delete[] temp;
  return len;
}

void AsyncWebSocket::text(uint32_t id, const char * message){
  text(id, message, strlen(message));
}
void AsyncWebSocket::text(uint32_t id, uint8_t * message, size_t len){
  text(id, (const char *)message, len);
}
void AsyncWebSocket::text(uint32_t id, char * message){
  text(id, message, strlen(message));
}
void AsyncWebSocket::text(uint32_t id, const String &message){
  text(id, message.c_str(), message.length());
}
void AsyncWebSocket::text(uint32_t id, const __FlashStringHelper *message){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL)
    c->text(message);
}
void AsyncWebSocket::textAll(const char * message){
  textAll(message, strlen(message));
}
void AsyncWebSocket::textAll(uint8_t * message, size_t len){
  textAll((const char *)message, len);
}
void AsyncWebSocket::textAll(char * message){
  textAll(message, strlen(message));
}
void AsyncWebSocket::textAll(const String &message){
  textAll(message.c_str(), message.length());
}
void AsyncWebSocket::textAll(const __FlashStringHelper *message){
  AsyncWebSocketClient * c = _clients;
  while(c != NULL){
    if(c->status() == WS_CONNECTED)
      c->text(message);
    c = c->next;
  }
}
void AsyncWebSocket::binary(uint32_t id, const char * message){
  binary(id, message, strlen(message));
}
void AsyncWebSocket::binary(uint32_t id, uint8_t * message, size_t len){
  binary(id, (const char *)message, len);
}
void AsyncWebSocket::binary(uint32_t id, char * message){
  binary(id, message, strlen(message));
}
void AsyncWebSocket::binary(uint32_t id, const String &message){
  binary(id, message.c_str(), message.length());
}
void AsyncWebSocket::binary(uint32_t id, const __FlashStringHelper *message, size_t len){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL)
    c-> binary(message, len);
}
void AsyncWebSocket::binaryAll(const char * message){
  binaryAll(message, strlen(message));
}
void AsyncWebSocket::binaryAll(uint8_t * message, size_t len){
  binaryAll((const char *)message, len);
}
void AsyncWebSocket::binaryAll(char * message){
  binaryAll(message, strlen(message));
}
void AsyncWebSocket::binaryAll(const String &message){
  binaryAll(message.c_str(), message.length());
}
void AsyncWebSocket::binaryAll(const __FlashStringHelper *message, size_t len){
  AsyncWebSocketClient * c = _clients;
  while(c != NULL){
    if(c->status() == WS_CONNECTED)
      c-> binary(message, len);
    c = c->next;
  }
 }

const char * WS_STR_CONNECTION = "Connection";
const char * WS_STR_UPGRADE = "Upgrade";
const char * WS_STR_ORIGIN = "Origin";
const char * WS_STR_VERSION = "Sec-WebSocket-Version";
const char * WS_STR_KEY = "Sec-WebSocket-Key";
const char * WS_STR_PROTOCOL = "Sec-WebSocket-Protocol";
const char * WS_STR_ACCEPT = "Sec-WebSocket-Accept";
const char * WS_STR_UUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

bool AsyncWebSocket::canHandle(AsyncWebServerRequest *request){
  if(!_enabled)
    return false;

  if(request->method() != HTTP_GET || !request->url().equals(_url))
    return false;

  request->addInterestingHeader(WS_STR_CONNECTION);
  request->addInterestingHeader(WS_STR_UPGRADE);
  request->addInterestingHeader(WS_STR_ORIGIN);
  request->addInterestingHeader(WS_STR_VERSION);
  request->addInterestingHeader(WS_STR_KEY);
  request->addInterestingHeader(WS_STR_PROTOCOL);
  return true;
}

void AsyncWebSocket::handleRequest(AsyncWebServerRequest *request){
  if(!request->hasHeader(WS_STR_VERSION) || !request->hasHeader(WS_STR_KEY)){
    request->send(400);
    return;
  }
  AsyncWebHeader* version = request->getHeader(WS_STR_VERSION);
  if(version->value().toInt() != 13){
    AsyncWebServerResponse *response = request->beginResponse(400);
    response->addHeader(WS_STR_VERSION,"13");
    request->send(response);
    return;
  }
  AsyncWebHeader* key = request->getHeader(WS_STR_KEY);
  AsyncWebServerResponse *response = new AsyncWebSocketResponse(key->value(), this);
  if(request->hasHeader(WS_STR_PROTOCOL)){
    AsyncWebHeader* protocol = request->getHeader(WS_STR_PROTOCOL);
    //ToDo: check protocol
    response->addHeader(WS_STR_PROTOCOL, protocol->value());
  }
  request->send(response);
}


/*
 * Response to Web Socket request - sends the authorization and detaches the TCP Client from the web server
 * Authentication code from https://github.com/Links2004/arduinoWebSockets/blob/master/src/WebSockets.cpp#L480
 */

AsyncWebSocketResponse::AsyncWebSocketResponse(String key, AsyncWebSocket *server){
  _server = server;
  _code = 101;
  uint8_t * hash = (uint8_t*)malloc(20);
  if(hash == NULL){
    _state = RESPONSE_FAILED;
    return;
  }
  char * buffer = (char *) malloc(33);
  if(buffer == NULL){
    free(hash);
    _state = RESPONSE_FAILED;
    return;
  }
#ifdef ESP8266
  sha1(key + WS_STR_UUID, hash);
#else
  key += WS_STR_UUID;
  SHA1_CTX ctx;
  SHA1Init(&ctx);
  SHA1Update(&ctx, (const unsigned char*)key.c_str(), key.length());
  SHA1Final(hash, &ctx);
#endif
  base64_encodestate _state;
  base64_init_encodestate(&_state);
  int len = base64_encode_block((const char *) hash, 20, buffer, &_state);
  len = base64_encode_blockend((buffer + len), &_state);
  addHeader(WS_STR_CONNECTION, WS_STR_UPGRADE);
  addHeader(WS_STR_UPGRADE, "websocket");
  addHeader(WS_STR_ACCEPT,buffer);
  free(buffer);
  free(hash);
}

void AsyncWebSocketResponse::_respond(AsyncWebServerRequest *request){
  if(_state == RESPONSE_FAILED){
    request->client()->close(true);
    return;
  }
  String out = _assembleHead(request->version());
  request->client()->write(out.c_str(), _headLength);
  _state = RESPONSE_WAIT_ACK;
}

size_t AsyncWebSocketResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time){
  if(len){
    new AsyncWebSocketClient(request, _server);
  }
  return 0;
}
