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
#if defined(ESP32)
  #include <rom/ets_sys.h>
#endif
#include "AsyncEventSource.h"

using namespace asyncsrv;

static String generateEventMessage(const char* message, const char* event, uint32_t id, uint32_t reconnect) {
  String ev;

  if (reconnect) {
    ev += T_retry_;
    ev += reconnect;
    ev += T_rn;
  }

  if (id) {
    ev += T_id__;
    ev += id;
    ev += T_rn;
  }

  if (event != NULL) {
    ev += T_event_;
    ev += event;
    ev += T_rn;
  }

  if (message != NULL) {
    size_t messageLen = strlen(message);
    char* lineStart = (char*)message;
    char* lineEnd;
    do {
      char* nextN = strchr(lineStart, '\n');
      char* nextR = strchr(lineStart, '\r');
      if (nextN == NULL && nextR == NULL) {
        size_t llen = ((char*)message + messageLen) - lineStart;
        char* ldata = (char*)malloc(llen + 1);
        if (ldata != NULL) {
          memcpy(ldata, lineStart, llen);
          ldata[llen] = 0;
          ev += T_data_;
          ev += ldata;
          ev += T_rnrn;
          free(ldata);
        }
        lineStart = (char*)message + messageLen;
      } else {
        char* nextLine = NULL;
        if (nextN != NULL && nextR != NULL) {
          if (nextR < nextN) {
            lineEnd = nextR;
            if (nextN == (nextR + 1))
              nextLine = nextN + 1;
            else
              nextLine = nextR + 1;
          } else {
            lineEnd = nextN;
            if (nextR == (nextN + 1))
              nextLine = nextR + 1;
            else
              nextLine = nextN + 1;
          }
        } else if (nextN != NULL) {
          lineEnd = nextN;
          nextLine = nextN + 1;
        } else {
          lineEnd = nextR;
          nextLine = nextR + 1;
        }

        size_t llen = lineEnd - lineStart;
        char* ldata = (char*)malloc(llen + 1);
        if (ldata != NULL) {
          memcpy(ldata, lineStart, llen);
          ldata[llen] = 0;
          ev += T_data_;
          ev += ldata;
          ev += T_rn;
          free(ldata);
        }
        lineStart = nextLine;
        if (lineStart == ((char*)message + messageLen))
          ev += T_rn;
      }
    } while (lineStart < ((char*)message + messageLen));
  }

  return ev;
}

// Message

AsyncEventSourceMessage::AsyncEventSourceMessage(const char* data, size_t len)
    : _data(nullptr), _len(len), _sent(0), _acked(0) {
  _data = (uint8_t*)malloc(_len + 1);
  if (_data == nullptr) {
    _len = 0;
  } else {
    memcpy(_data, data, len);
    _data[_len] = 0;
  }
}

AsyncEventSourceMessage::~AsyncEventSourceMessage() {
  if (_data != NULL)
    free(_data);
}

size_t AsyncEventSourceMessage::ack(size_t len, __attribute__((unused)) uint32_t time) {
  // If the whole message is now acked...
  if (_acked + len > _len) {
    // Return the number of extra bytes acked (they will be carried on to the next message)
    const size_t extra = _acked + len - _len;
    _acked = _len;
    return extra;
  }
  // Return that no extra bytes left.
  _acked += len;
  return 0;
}

size_t AsyncEventSourceMessage::write(AsyncClient* client) {
  if (!client)
    return 0;

  if (_sent >= _len || !client->canSend()) {
    return 0;
  }
  size_t len = min(_len - _sent, client->space());
  size_t sent = client->add((const char*)_data + _sent, len);
  _sent += sent;
  return sent;
}

size_t AsyncEventSourceMessage::send(AsyncClient* client) {
  size_t sent = write(client);
  return sent && client->send() ? sent : 0;
}

// Client

AsyncEventSourceClient::AsyncEventSourceClient(AsyncWebServerRequest* request, AsyncEventSource* server) {
  _client = request->client();
  _server = server;
  _lastId = 0;
  if (request->hasHeader(T_Last_Event_ID))
    _lastId = atoi(request->getHeader(T_Last_Event_ID)->value().c_str());

  _client->setRxTimeout(0);
  _client->onError(NULL, NULL);
  _client->onAck([](void* r, AsyncClient* c, size_t len, uint32_t time) { (void)c; ((AsyncEventSourceClient*)(r))->_onAck(len, time); }, this);
  _client->onPoll([](void* r, AsyncClient* c) { (void)c; ((AsyncEventSourceClient*)(r))->_onPoll(); }, this);
  _client->onData(NULL, NULL);
  _client->onTimeout([this](void* r, AsyncClient* c __attribute__((unused)), uint32_t time) { ((AsyncEventSourceClient*)(r))->_onTimeout(time); }, this);
  _client->onDisconnect([this](void* r, AsyncClient* c) { ((AsyncEventSourceClient*)(r))->_onDisconnect(); delete c; }, this);

  _server->_addClient(this);
  delete request;

  _client->setNoDelay(true);
}

AsyncEventSourceClient::~AsyncEventSourceClient() {
#ifdef ESP32
  std::lock_guard<std::mutex> lock(_lockmq);
#endif
  _messageQueue.clear();
  close();
}

bool AsyncEventSourceClient::_queueMessage(const char* message, size_t len) {
  if (!_client)
    return false;

#ifdef ESP32
  // length() is not thread-safe, thus acquiring the lock before this call..
  std::lock_guard<std::mutex> lock(_lockmq);
#endif

  if (_messageQueue.size() >= SSE_MAX_QUEUED_MESSAGES) {
#ifdef ESP8266
    ets_printf(String(F("ERROR: Too many messages queued\n")).c_str());
#elif defined(ESP32)
    log_e("Too many messages queued: deleting message");
#endif
    return false;
  }

  _messageQueue.emplace_back(message, len);
  // runqueue trigger when new messages added
  if (_client->canSend()) {
    _runQueue();
  }

  return true;
}

void AsyncEventSourceClient::_onAck(size_t len __attribute__((unused)), uint32_t time __attribute__((unused))) {
#ifdef ESP32
  // Same here, acquiring the lock early
  std::lock_guard<std::mutex> lock(_lockmq);
#endif
  _runQueue();
}

void AsyncEventSourceClient::_onPoll() {
#ifdef ESP32
  // Same here, acquiring the lock early
  std::lock_guard<std::mutex> lock(_lockmq);
#endif
  if (_messageQueue.size()) {
    _runQueue();
  }
}

void AsyncEventSourceClient::_onTimeout(uint32_t time __attribute__((unused))) {
  if (_client)
    _client->close(true);
}

void AsyncEventSourceClient::_onDisconnect() {
  if (!_client)
    return;
  _client = nullptr;
  _server->_handleDisconnect(this);
}

void AsyncEventSourceClient::close() {
  if (_client)
    _client->close();
}

bool AsyncEventSourceClient::write(const char* message, size_t len) {
  return connected() && _queueMessage(message, len);
}

bool AsyncEventSourceClient::send(const char* message, const char* event, uint32_t id, uint32_t reconnect) {
  if (!connected())
    return false;
  String ev = generateEventMessage(message, event, id, reconnect);
  return _queueMessage(ev.c_str(), ev.length());
}

size_t AsyncEventSourceClient::packetsWaiting() const {
#ifdef ESP32
  std::lock_guard<std::mutex> lock(_lockmq);
#endif
  return _messageQueue.size();
}

void AsyncEventSourceClient::_runQueue() {
  if (!_client)
    return;

  size_t total_bytes_written = 0;
  for (auto i = _messageQueue.begin(); i != _messageQueue.end(); ++i) {
    if (!i->sent()) {
      const size_t bytes_written = i->write(_client);
      total_bytes_written += bytes_written;
      if (bytes_written == 0)
        break;
    }
  }

  if (total_bytes_written > 0)
    _client->send();

  size_t len = total_bytes_written;
  while (len && _messageQueue.size()) {
    len = _messageQueue.front().ack(len);
    if (_messageQueue.front().finished()) {
      _messageQueue.pop_front();
    }
  }
}

void AsyncEventSource::authorizeConnect(ArAuthorizeConnectHandler cb) {
  AuthorizationMiddleware* m = new AuthorizationMiddleware(401, cb);
  m->_freeOnRemoval = true;
  addMiddleware(m);
}

void AsyncEventSource::_addClient(AsyncEventSourceClient* client) {
  if (!client)
    return;
#ifdef ESP32
  std::lock_guard<std::mutex> lock(_client_queue_lock);
#endif
  _clients.emplace_back(client);
  if (_connectcb)
    _connectcb(client);
}

void AsyncEventSource::_handleDisconnect(AsyncEventSourceClient* client) {
#ifdef ESP32
  std::lock_guard<std::mutex> lock(_client_queue_lock);
#endif
  if (_disconnectcb)
    _disconnectcb(client);
  for (auto i = _clients.begin(); i != _clients.end(); ++i) {
    if (i->get() == client)
      _clients.erase(i);
  }
}

void AsyncEventSource::close() {
  // While the whole loop is not done, the linked list is locked and so the
  // iterator should remain valid even when AsyncEventSource::_handleDisconnect()
  // is called very early
#ifdef ESP32
  std::lock_guard<std::mutex> lock(_client_queue_lock);
#endif
  for (const auto& c : _clients) {
    if (c->connected())
      c->close();
  }
}

// pmb fix
size_t AsyncEventSource::avgPacketsWaiting() const {
  size_t aql = 0;
  uint32_t nConnectedClients = 0;
#ifdef ESP32
  std::lock_guard<std::mutex> lock(_client_queue_lock);
#endif
  if (!_clients.size())
    return 0;

  for (const auto& c : _clients) {
    if (c->connected()) {
      aql += c->packetsWaiting();
      ++nConnectedClients;
    }
  }
  return ((aql) + (nConnectedClients / 2)) / (nConnectedClients); // round up
}

AsyncEventSource::SendStatus AsyncEventSource::send(
  const char* message, const char* event, uint32_t id, uint32_t reconnect) {
  String ev = generateEventMessage(message, event, id, reconnect);
#ifdef ESP32
  std::lock_guard<std::mutex> lock(_client_queue_lock);
#endif
  size_t hits = 0;
  size_t miss = 0;
  for (const auto& c : _clients) {
    if (c->write(ev.c_str(), ev.length()))
      ++hits;
    else
      ++miss;
  }
  return hits == 0 ? DISCARDED : (miss == 0 ? ENQUEUED : PARTIALLY_ENQUEUED);
}

size_t AsyncEventSource::count() const {
#ifdef ESP32
  std::lock_guard<std::mutex> lock(_client_queue_lock);
#endif
  size_t n_clients{0};
  for (const auto& i : _clients)
    if (i->connected())
      ++n_clients;

  return n_clients;
}

bool AsyncEventSource::canHandle(AsyncWebServerRequest* request) const {
  return request->isSSE() && request->url().equals(_url);
}

void AsyncEventSource::handleRequest(AsyncWebServerRequest* request) {
  request->send(new AsyncEventSourceResponse(this));
}

// Response

AsyncEventSourceResponse::AsyncEventSourceResponse(AsyncEventSource* server) {
  _server = server;
  _code = 200;
  _contentType = T_text_event_stream;
  _sendContentLength = false;
  addHeader(T_Cache_Control, T_no_cache);
  addHeader(T_Connection, T_keep_alive);
}

void AsyncEventSourceResponse::_respond(AsyncWebServerRequest* request) {
  String out;
  _assembleHead(out, request->version());
  request->client()->write(out.c_str(), _headLength);
  _state = RESPONSE_WAIT_ACK;
}

size_t AsyncEventSourceResponse::_ack(AsyncWebServerRequest* request, size_t len, uint32_t time __attribute__((unused))) {
  if (len) {
    new AsyncEventSourceClient(request, _server);
  }
  return 0;
}
