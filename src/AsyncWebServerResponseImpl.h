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

class AsyncFileResponse: public AsyncWebServerResponse {
  private:
    File _content;
    String _path;
    String _head;
    void _setContentType(String path);
  public:
    AsyncFileResponse(FS &fs, String path, String contentType=String(), bool download=false);
    ~AsyncFileResponse();
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
};

class AsyncStreamResponse: public AsyncWebServerResponse {
  private:
    Stream *_content;
    String _head;
  public:
    AsyncStreamResponse(Stream &stream, String contentType, size_t len);
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
};

class AsyncCallbackResponse: public AsyncWebServerResponse {
  private:
    AwsResponseFiller _content;
    String _head;
  public:
    AsyncCallbackResponse(String contentType, size_t len, AwsResponseFiller callback);
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
};

#endif /* ASYNCWEBSERVERRESPONSEIMPL_H_ */
