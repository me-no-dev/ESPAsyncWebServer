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
#ifndef ASYNCWEBSERVERRESPONSEIMPL_H_
#define ASYNCWEBSERVERRESPONSEIMPL_H_

class AsyncBasicResponse: public AsyncWebServerResponse {
  private:
    String _content;
  public:
    AsyncBasicResponse(int code, String contentType=String(), String content=String());
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
    bool _sourceValid(){ return true; }
};

class AsyncAbstractResponse: public AsyncWebServerResponse {
  private:
    String _head;
  public:
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
    bool _sourceValid(){ return false; }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen){ return 0; }
};

class AsyncFileResponse: public AsyncAbstractResponse {
  private:
    File _content;
    String _path;
    void _setContentType(String path);
  public:
    AsyncFileResponse(FS &fs, String path, String contentType=String(), bool download=false);
    AsyncFileResponse(File content, String path, String contentType=String(), bool download=false);
    ~AsyncFileResponse();
    bool _sourceValid(){ return !!(_content); }
    size_t _fillBuffer(uint8_t *buf, size_t maxLen);
};

class AsyncStreamResponse: public AsyncAbstractResponse {
  private:
    Stream *_content;
  public:
    AsyncStreamResponse(Stream &stream, String contentType, size_t len);
    bool _sourceValid(){ return !!(_content); }
    size_t _fillBuffer(uint8_t *buf, size_t maxLen);
};

class AsyncCallbackResponse: public AsyncAbstractResponse {
  private:
    AwsResponseFiller _content;
  public:
    AsyncCallbackResponse(String contentType, size_t len, AwsResponseFiller callback);
    bool _sourceValid(){ return !!(_content); }
    size_t _fillBuffer(uint8_t *buf, size_t maxLen);
};

class AsyncChunkedResponse: public AsyncAbstractResponse {
  private:
    AwsResponseFiller _content;
  public:
    AsyncChunkedResponse(String contentType, AwsResponseFiller callback);
    bool _sourceValid(){ return !!(_content); }
    size_t _fillBuffer(uint8_t *buf, size_t maxLen);
};

class AsyncProgmemResponse: public AsyncAbstractResponse {
  private:
    const uint8_t * _content;
  public:
    AsyncProgmemResponse(int code, String contentType, const uint8_t * content, size_t len);
    bool _sourceValid(){ return true; }
    size_t _fillBuffer(uint8_t *buf, size_t maxLen);
};

class cbuf;

class AsyncResponseStream: public AsyncAbstractResponse, public Print {
  private:
    cbuf *_content;
  public:
    AsyncResponseStream(String contentType, size_t bufferSize);
    ~AsyncResponseStream();
    bool _sourceValid(){ return (_state < RESPONSE_END); }
    size_t _fillBuffer(uint8_t *buf, size_t maxLen);
    size_t write(const uint8_t *data, size_t len);
    size_t write(uint8_t data);
    using Print::write;
};

#endif /* ASYNCWEBSERVERRESPONSEIMPL_H_ */
