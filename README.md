# ESP Async WebServer

[![License: LGPL 3.0](https://img.shields.io/badge/License-LGPL%203.0-yellow.svg)](https://opensource.org/license/lgpl-3-0/)
[![Continuous Integration](https://github.com/mathieucarbou/ESPAsyncWebServer/actions/workflows/ci.yml/badge.svg)](https://github.com/mathieucarbou/ESPAsyncWebServer/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/mathieucarbou/library/ESP%20Async%20WebServer.svg)](https://registry.platformio.org/libraries/mathieucarbou/ESP%20Async%20WebServer)

Async Web Server for ESP31B

This fork is based on https://github.com/yubox-node-org/ESPAsyncWebServer and includes all the concurrency fixes.

## Changes

- SPIFFSEditor is removed
- Arduino Json 7 compatibility
- Deployed in PlatformIO registry and Arduino IDE library manager
- CI
- Only supports ESP32

## Documentation

Usage and API stays the same as the original library.
Please look at the original libraries for more examples and documentation.

[https://github.com/yubox-node-org/ESPAsyncWebServer](https://github.com/yubox-node-org/ESPAsyncWebServer)

## Pitfalls

The fork from yubox introduces some breaking API changes compared to the original library, especially regarding the use of `std::shared_ptr<std::vector<uint8_t>>` for WebSocket.
Thanks to this fork, you can handle them by using `ASYNCWEBSERVER_FORK_mathieucarbou`.

Here is an example for serializing a Json document in a websocket message buffer directly.
This code is compatible with both forks.

```cpp
void send(JsonDocument& doc) {
  const size_t len = measureJson(doc);

#if defined(ASYNCWEBSERVER_FORK_mathieucarbou)

  // this fork (originally from yubox-node-org), uses another API with shared pointer that better support concurrent use cases then the original project
  auto buffer = std::make_shared<std::vector<uint8_t>>(len);
  assert(buffer); // up to you to keep or remove this
  serializeJson(doc, buffer->data(), len);
  _ws->textAll(std::move(buffer));

#else

  // original API from me-no-dev
  AsyncWebSocketMessageBuffer* buffer = _ws->makeBuffer(len);
  assert(buffer); // up to you to keep or remove this
  serializeJson(doc, buffer->get(), len);
  _ws->textAll(buffer);

#endif
}
```
