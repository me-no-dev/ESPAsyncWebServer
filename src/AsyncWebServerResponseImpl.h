/*
 * AsyncWebImpl.h
 *
 *  Created on: 19.12.2015 г.
 *      Author: ficeto
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
};

class AsyncAbstractResponse: public AsyncWebServerResponse {
  private:
    String _head;
  public:
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen){ return 0; }
    virtual bool _sourceValid(){ return false; }
};

class AsyncFileResponse: public AsyncAbstractResponse {
  private:
    File _content;
    String _path;
    void _setContentType(String path);
  public:
    AsyncFileResponse(FS &fs, String path, String contentType=String(), bool download=false);
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

class AsyncResponseStream: public AsyncAbstractResponse, public Print {
  private:
    cbuf *_content;
  public:
    AsyncResponseStream(String contentType, size_t len);
    ~AsyncResponseStream();
    bool _sourceValid(){ return (_state < RESPONSE_END); }
    size_t _fillBuffer(uint8_t *buf, size_t maxLen);
    size_t write(const uint8_t *data, size_t len);
    size_t write(uint8_t data);
    using Print::write;
};

#endif /* ASYNCWEBSERVERRESPONSEIMPL_H_ */
