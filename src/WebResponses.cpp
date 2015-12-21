#include "ESPAsyncWebServer.h"
#include "AsyncWebServerResponseImpl.h"

/*
 * Abstract Response
 * */
const char* AsyncWebServerResponse::_responseCodeToString(int code) {
  switch (code) {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-Authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 307: return "Temporary Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Time-out";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Request Entity Too Large";
    case 414: return "Request-URI Too Large";
    case 415: return "Unsupported Media Type";
    case 416: return "Requested range not satisfiable";
    case 417: return "Expectation Failed";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Time-out";
    case 505: return "HTTP Version not supported";
    default:  return "";
  }
}

AsyncWebServerResponse::AsyncWebServerResponse()
:_code(0)
,_headers(NULL)
,_contentType()
,_contentLength(0)
,_headLength(0)
,_sentLength(0)
,_ackedLength(0)
,_state(RESPONSE_SETUP)
{
  addHeader("Connection","close");
  addHeader("Access-Control-Allow-Origin","*");
}

AsyncWebServerResponse::~AsyncWebServerResponse(){
  while(_headers != NULL){
    AsyncWebHeader *h = _headers;
    _headers = h->next;
    delete h;
  }
}

void AsyncWebServerResponse::addHeader(String name, String value){
  AsyncWebHeader *header = new AsyncWebHeader(name, value);
  if(_headers == NULL){
    _headers = header;
  } else {
    AsyncWebHeader *h = _headers;
    while(h->next != NULL) h = h->next;
    h->next = header;
  }
}

String AsyncWebServerResponse::_assembleHead(){
  String out = "HTTP/1.1 " + String(_code) + " " + _responseCodeToString(_code) + "\r\n";
  out += "Content-Length: " + String(_contentLength) + "\r\n";
  if(_contentType.length()){
    out += "Content-Type: " + _contentType + "\r\n";
  }
  AsyncWebHeader *h;
  while(_headers != NULL){
    h = _headers;
    _headers = _headers->next;
    out += h->toString();
    delete h;
  }
  out += "\r\n";
  _headLength = out.length();
  return out;
}

bool AsyncWebServerResponse::_finished(){ return _state > RESPONSE_WAIT_ACK; }
bool AsyncWebServerResponse::_failed(){ return _state == RESPONSE_FAILED; }
void AsyncWebServerResponse::_respond(AsyncWebServerRequest *request){ _state = RESPONSE_END; request->client()->close(); }
size_t AsyncWebServerResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time){ return 0; }

/*
 * String/Code Response
 * */
AsyncBasicResponse::AsyncBasicResponse(int code, String contentType, String content){
  _code = code;
  _content = content;
  _contentType = contentType;
  if(_content.length()){
    _contentLength = _content.length();
    if(!_contentType.length())
      _contentType = "text/plain";
  }
}

void AsyncBasicResponse::_respond(AsyncWebServerRequest *request){
  _state = RESPONSE_HEADERS;
  String out = _assembleHead();
  size_t outLen = out.length();
  size_t space = request->client()->space();
  if(!_contentLength && space >= outLen){
    request->client()->write(out.c_str(), outLen);
    _state = RESPONSE_WAIT_ACK;
  } else if(_contentLength && space >= outLen + _contentLength){
    out += _content;
    outLen += _contentLength;
    request->client()->write(out.c_str(), outLen);
    _state = RESPONSE_WAIT_ACK;
  } else if(space && space < out.length()){
    String partial = out.substring(0, space);
    _content = out.substring(space) + _content;
    _contentLength += outLen - space;
    request->client()->write(partial.c_str(), partial.length());
    _state = RESPONSE_CONTENT;
  } else if(space > outLen && space < (outLen + _contentLength)){
    size_t shift = space - outLen;
    outLen += shift;
    _sentLength += shift;
    out += _content.substring(0, shift);
    _content = _content.substring(shift);
    request->client()->write(out.c_str(), outLen);
    _state = RESPONSE_CONTENT;
  } else {
    _content = out + _content;
    _contentLength += outLen;
    _state = RESPONSE_CONTENT;
  }
}

size_t AsyncBasicResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time){
  _ackedLength += len;
  if(_state == RESPONSE_CONTENT){
    size_t available = _contentLength - _sentLength;
    size_t space = request->client()->space();
    //we can fit in this packet
    if(space > available){
      request->client()->write(_content.c_str(), available);
      _content = String();
      _state = RESPONSE_WAIT_ACK;
      return available;
    }
    //send some data, the rest on ack
    String out = _content.substring(0, space);
    _content = _content.substring(space);
    _sentLength += space;
    request->client()->write(out.c_str(), space);
    return space;
  } else if(_state == RESPONSE_WAIT_ACK){
    if(_ackedLength >= (_headLength+_contentLength)){
      _state = RESPONSE_END;
    }
  }
  return 0;
}


/*
 * Abstract Response
 * */

void AsyncAbstractResponse::_respond(AsyncWebServerRequest *request){
  if(!_sourceValid()){
    _state = RESPONSE_FAILED;
    request->send(500);
    return;
  }
  _head = _assembleHead();
  _state = RESPONSE_HEADERS;
  size_t outLen = _head.length();
  size_t space = request->client()->space();
  if(space >= outLen){
    request->client()->write(_head.c_str(), outLen);
    _head = String();
    _state = RESPONSE_CONTENT;
  } else {
    String out = _head.substring(0, space);
    _head = _head.substring(space);
    request->client()->write(out.c_str(), out.length());
    out = String();
  }
}

size_t AsyncAbstractResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time){
  if(!_sourceValid()){
    _state = RESPONSE_FAILED;
    request->client()->close();
    return 0;
  }
  _ackedLength += len;
  size_t space = request->client()->space();
  if(_state == RESPONSE_CONTENT){
    size_t remaining = _contentLength - _sentLength;
    size_t outLen = (remaining > space)?space:remaining;
    uint8_t *buf = (uint8_t *)malloc(outLen);
    outLen = _fillBuffer(buf, outLen);
    request->client()->write((const char*)buf, outLen);
    _sentLength += outLen;
    free(buf);
    if(_sentLength == _contentLength){
      _state = RESPONSE_WAIT_ACK;
    }
    return outLen;
  } else if(_state == RESPONSE_HEADERS){
    size_t outLen = _head.length();
    if(space >= outLen){
      request->client()->write(_head.c_str(), outLen);
      _head = String();
      _state = RESPONSE_CONTENT;
      return outLen;
    } else {
      String out = _head.substring(0, space);
      _head = _head.substring(space);
      request->client()->write(out.c_str(), out.length());
      return out.length();
    }
  } else if(_state == RESPONSE_WAIT_ACK){
    if(_ackedLength >= (_headLength+_contentLength)){
      _state = RESPONSE_END;
    }
  }
  return 0;
}




/*
 * File Response
 * */

AsyncFileResponse::~AsyncFileResponse(){
  if(_content)
    _content.close();
}

void AsyncFileResponse::_setContentType(String path){
  if (path.endsWith(".html")) _contentType = "text/html";
  else if (path.endsWith(".htm")) _contentType = "text/html";
  else if (path.endsWith(".css")) _contentType = "text/css";
  else if (path.endsWith(".txt")) _contentType = "text/plain";
  else if (path.endsWith(".js")) _contentType = "application/javascript";
  else if (path.endsWith(".png")) _contentType = "image/png";
  else if (path.endsWith(".gif")) _contentType = "image/gif";
  else if (path.endsWith(".jpg")) _contentType = "image/jpeg";
  else if (path.endsWith(".ico")) _contentType = "image/x-icon";
  else if (path.endsWith(".svg")) _contentType = "image/svg+xml";
  else if (path.endsWith(".xml")) _contentType = "text/xml";
  else if (path.endsWith(".pdf")) _contentType = "application/pdf";
  else if (path.endsWith(".zip")) _contentType = "application/zip";
  else if(path.endsWith(".gz")) _contentType = "application/x-gzip";
  else _contentType = "application/octet-stream";
}

AsyncFileResponse::AsyncFileResponse(FS &fs, String path, String contentType, bool download){
  _code = 200;
  _path = path;
  if(!download && !fs.exists(_path) && fs.exists(_path+".gz")){
    _path = _path+".gz";
    addHeader("Content-Encoding", "gzip");
  }

  if(download)
    _contentType = "application/octet-stream";
  else
    _setContentType(path);
  _content = fs.open(_path, "r");
  _contentLength = _content.size();
}

size_t AsyncFileResponse::_fillBuffer(uint8_t *data, size_t len){
  _content.read(data, len);
  return len;
}

/*
 * Stream Response
 * */

AsyncStreamResponse::AsyncStreamResponse(Stream &stream, String contentType, size_t len){
  _code = 200;
  _content = &stream;
  _contentLength = len;
  _contentType = contentType;
}

size_t AsyncStreamResponse::_fillBuffer(uint8_t *data, size_t len){
  size_t available = _content->available();
  size_t outLen = (available > len)?len:available;
  size_t i;
  for(i=0;i<outLen;i++)
    data[i] = _content->read();
  return outLen;
}

/*
 * Callback Response
 * */

AsyncCallbackResponse::AsyncCallbackResponse(String contentType, size_t len, AwsResponseFiller callback){
  _code = 200;
  _content = callback;
  _contentLength = len;
  _contentType = contentType;
}

size_t AsyncCallbackResponse::_fillBuffer(uint8_t *data, size_t len){
  return _content(data, len);
}




