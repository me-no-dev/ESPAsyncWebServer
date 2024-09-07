#include "ESPAsyncWebServer.h"

void LoggingMiddleware::run(AsyncWebServerRequest* request, ArMiddlewareNext next) {
  if (!isEnabled()) {
    next();
    return;
  }
  _out->print(F("* Connection from "));
  _out->print(request->client()->remoteIP().toString());
  _out->print(':');
  _out->println(request->client()->remotePort());
  _out->print('>');
  _out->print(' ');
  _out->print(request->methodToString());
  _out->print(' ');
  _out->print(request->url().c_str());
  _out->print(F(" HTTP/1."));
  _out->println(request->version());
  for (auto& h : request->getHeaders()) {
    if (h.value().length()) {
      _out->print('>');
      _out->print(' ');
      _out->print(h.name());
      _out->print(':');
      _out->print(' ');
      _out->println(h.value());
    }
  }
  _out->println(F(">"));
  uint32_t elapsed = millis();
  next();
  elapsed = millis() - elapsed;
  AsyncWebServerResponse* response = request->getResponse();
  if (response) {
    _out->print(F("* Processed in "));
    _out->print(elapsed);
    _out->println(F(" ms"));
    _out->print('<');
    _out->print(F(" HTTP/1."));
    _out->print(request->version());
    _out->print(' ');
    _out->print(response->code());
    _out->print(' ');
    _out->println(AsyncWebServerResponse::responseCodeToString(response->code()));
    for (auto& h : response->getHeaders()) {
      if (h.value().length()) {
        _out->print('<');
        _out->print(' ');
        _out->print(h.name());
        _out->print(':');
        _out->print(' ');
        _out->println(h.value());
      }
    }
    _out->println('<');
  } else {
    _out->println(F("* Connection closed!"));
  }
}

void CorsMiddleware::run(AsyncWebServerRequest* request, ArMiddlewareNext next) {
  if (request->method() == HTTP_OPTIONS && request->hasHeader(F("Origin"))) {
    AsyncWebServerResponse* response = request->beginResponse(200);
    response->addHeader(F("Access-Control-Allow-Origin"), _origin.c_str());
    response->addHeader(F("Access-Control-Allow-Methods"), _methods.c_str());
    response->addHeader(F("Access-Control-Allow-Headers"), _headers.c_str());
    response->addHeader(F("Access-Control-Allow-Credentials"), _credentials ? F("true") : F("false"));
    response->addHeader(F("Access-Control-Max-Age"), String(_maxAge).c_str());
    request->send(response);
  } else {
    next();
  }
}

void RateLimitMiddleware::run(AsyncWebServerRequest* request, ArMiddlewareNext next) {
  uint32_t retryAfterSeconds;
  if (isRequestAllowed(retryAfterSeconds)) {
    next();
  } else {
    AsyncWebServerResponse* response = request->beginResponse(429);
    response->addHeader(F("Retry-After"), retryAfterSeconds);
    request->send(response);
  }
}
