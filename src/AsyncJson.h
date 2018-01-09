// ESPasyncJson.h
/*
  Async Response to use with arduinoJson and asyncwebserver
  Written by Andrew Melvin (SticilFace) with help from me-no-dev and BBlanchon.

  example of callback in use

   server.on("/json", HTTP_ANY, [](AsyncWebServerRequest * request) {

    AsyncJsonResponse * response = new AsyncJsonResponse();
    JsonObject& root = response->getRoot();
    root["key1"] = "key number one";
    JsonObject& nested = root.createNestedObject("nested");
    nested["key1"] = "key number one";

    response->setLength();
    request->send(response);
  });

*/
#ifndef ASYNC_JSON_H_
#define ASYNC_JSON_H_
#include <ArduinoJson.h>

/*
 * Json Response
 * */

class ChunkPrint : public Print {
  private:
    uint8_t* _destination;
    size_t _to_skip;
    size_t _to_write;
    size_t _pos;
  public:
    ChunkPrint(uint8_t* destination, size_t from, size_t len)
      : _destination(destination), _to_skip(from), _to_write(len), _pos{0} {}
    virtual ~ChunkPrint(){}
    size_t write(uint8_t c){
      if (_to_skip > 0) {
        _to_skip--;
        return 1;
      } else if (_to_write > 0) {
        _to_write--;
        _destination[_pos++] = c;
        return 1;
      }
      return 0;
    }
};

class AsyncJsonResponse: public AsyncAbstractResponse {
  private:
    DynamicJsonBuffer _jsonBuffer;
    JsonVariant _root;
    bool _isValid;
  public:
    AsyncJsonResponse(bool isArray=false): _isValid{false} {
      _code = 200;
      _contentType = "application/json";
      if(isArray)
        _root = _jsonBuffer.createArray();
      else
        _root = _jsonBuffer.createObject();
    }
    ~AsyncJsonResponse() {}
    JsonVariant & getRoot() { return _root; }
    bool _sourceValid() const { return _isValid; }
    size_t setLength() {
      _contentLength = _root.measureLength();
      if (_contentLength) { _isValid = true; }
      return _contentLength;
    }

   size_t getSize() { return _jsonBuffer.size(); }

    size_t _fillBuffer(uint8_t *data, size_t len){
      ChunkPrint dest(data, _sentLength, len);
      _root.printTo( dest ) ;
      return len;
    }
};
#endif
