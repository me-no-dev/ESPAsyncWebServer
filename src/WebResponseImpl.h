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
    AsyncBasicResponse(int code, const String& contentType=String(), const String& content=String());
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
    bool _sourceValid() const { return true; }
};

class AsyncAbstractResponse: public AsyncWebServerResponse {
  private:
    String _head;
  public:
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
    bool _sourceValid() const { return false; }
    virtual size_t _fillBuffer(uint8_t *buf __attribute__((unused)), size_t maxLen __attribute__((unused))) { return 0; }
};

class AsyncFileResponse: public AsyncAbstractResponse {
  using File = fs::File;
  using FS = fs::FS;
  private:
    File _content;
    String _path;
    void _setContentType(const String& path);
  public:
    AsyncFileResponse(FS &fs, const String& path, const String& contentType=String(), bool download=false);
    AsyncFileResponse(File content, const String& path, const String& contentType=String(), bool download=false);
    ~AsyncFileResponse();
    bool _sourceValid() const { return !!(_content); }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
};

class AsyncStreamResponse: public AsyncAbstractResponse {
  private:
    Stream *_content;
  public:
    AsyncStreamResponse(Stream &stream, const String& contentType, size_t len);
    bool _sourceValid() const { return !!(_content); }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
};

class AsyncCallbackResponse: public AsyncAbstractResponse {
  private:
    AwsResponseFiller _content;
  public:
    AsyncCallbackResponse(const String& contentType, size_t len, AwsResponseFiller callback);
    bool _sourceValid() const { return !!(_content); }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
};

class AsyncChunkedResponse: public AsyncAbstractResponse {
  private:
    AwsResponseFiller _content;
  public:
    AsyncChunkedResponse(const String& contentType, AwsResponseFiller callback);
    bool _sourceValid() const { return !!(_content); }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
};

class AsyncProgmemResponse: public AsyncAbstractResponse {
  private:
    const uint8_t * _content;
  public:
    AsyncProgmemResponse(int code, const String& contentType, const uint8_t * content, size_t len);
    bool _sourceValid() const { return true; }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
};

class cbuf;

class AsyncResponseStream: public AsyncAbstractResponse, public Print {
  private:
    cbuf *_content;
  public:
    AsyncResponseStream(const String& contentType, size_t bufferSize);
    ~AsyncResponseStream();
    bool _sourceValid() const { return (_state < RESPONSE_END); }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
    size_t write(const uint8_t *data, size_t len);
    size_t write(uint8_t data);
    using Print::write;
};

#endif /* ASYNCWEBSERVERRESPONSEIMPL_H_ */
