# ESP Async WebServer

[![License: LGPL 3.0](https://img.shields.io/badge/License-LGPL%203.0-yellow.svg)](https://opensource.org/license/lgpl-3-0/)
[![Continuous Integration](https://github.com/mathieucarbou/ESPAsyncWebServer/actions/workflows/ci.yml/badge.svg)](https://github.com/mathieucarbou/ESPAsyncWebServer/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/mathieucarbou/library/ESP%20Async%20WebServer.svg)](https://registry.platformio.org/libraries/mathieucarbou/ESP%20Async%20WebServer)

Asynchronous HTTP and WebSocket Server Library for ESP32, ESP8266 and RP2040
Supports: WebSocket, SSE, Authentication, Arduino Json 7, File Upload, Static File serving, URL Rewrite, URL Redirect, etc.

This fork is based on [yubox-node-org/ESPAsyncWebServer](https://github.com/yubox-node-org/ESPAsyncWebServer) and includes all the concurrency fixes.

## Changes in this fork

- [@ayushsharma82](https://github.com/ayushsharma82) and [@mathieucarbou](https://github.com/mathieucarbou): Add RP2040 support ([#31](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/31))
- [@mathieucarbou](https://github.com/mathieucarbou): `SSE_MAX_QUEUED_MESSAGES` to control the maximum number of messages that can be queued for a SSE client
- [@mathieucarbou](https://github.com/mathieucarbou): `write()` function public in `AsyncEventSource.h`
- [@mathieucarbou](https://github.com/mathieucarbou): `WS_MAX_QUEUED_MESSAGES`: control the maximum number of messages that can be queued for a Websocket client
- [@mathieucarbou](https://github.com/mathieucarbou): Added `setAuthentication(const String& username, const String& password)`
- [@mathieucarbou](https://github.com/mathieucarbou): Added `setCloseClientOnQueueFull(bool)` which can be set on a client to either close the connection or discard messages but not close the connection when the queue is full
- [@mathieucarbou](https://github.com/mathieucarbou): Added `StreamConcat` example to show how to stream multiple files in one response
- [@mathieucarbou](https://github.com/mathieucarbou): Added all flavors of `binary()`, `text()`, `binaryAll()` and `textAll()` in `AsyncWebSocket`
- [@mathieucarbou](https://github.com/mathieucarbou): Arduino 3 / ESP-IDF 5.1 compatibility
- [@mathieucarbou](https://github.com/mathieucarbou): Arduino Json 7 compatibility and backward compatible with 6 and 6 (changes in `AsyncJson.h`). The API to use Json has not changed. These are only internal changes.
- [@mathieucarbou](https://github.com/mathieucarbou): CI
- [@mathieucarbou](https://github.com/mathieucarbou): Depends on `mathieucarbou/Async TCP @ ^3.1.4`
- [@mathieucarbou](https://github.com/mathieucarbou): Deployed in PlatformIO registry and Arduino IDE library manager
- [@mathieucarbou](https://github.com/mathieucarbou): Firmware size optimization: remove mbedtls dependency (accounts for 33KB in firmware)
- [@mathieucarbou](https://github.com/mathieucarbou): Made DEFAULT_MAX_SSE_CLIENTS customizable
- [@mathieucarbou](https://github.com/mathieucarbou): Made DEFAULT_MAX_WS_CLIENTS customizable
- [@mathieucarbou](https://github.com/mathieucarbou): Remove filename after inline in Content-Disposition header according to RFC2183
- [@mathieucarbou](https://github.com/mathieucarbou): Removed SPIFFSEditor to reduce library size and maintainance. SPIFF si also deprecated. If you need it, please copy the files from the original repository in your project. This fork focus on maintaining the server part and the SPIFFEditor is an application which has nothing to do inside a server library.
- [@mathieucarbou](https://github.com/mathieucarbou): Resurrected `AsyncWebSocketMessageBuffer` and `makeBuffer()` in order to make the fork API-compatible with the original library from me-no-dev regarding WebSocket.
- [@mathieucarbou](https://github.com/mathieucarbou): Some code cleanup
- [@mathieucarbou](https://github.com/mathieucarbou): Use `-D DEFAULT_MAX_WS_CLIENTS` to change the number of allows WebSocket clients and use `cleanupClients()` to help cleanup resources about dead clients
- [@nilo85](https://github.com/nilo85): Add support for Auth & GET requests in AsyncCallbackJsonWebHandler ([#14](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/14))
- [@p0p-x](https://github.com/p0p-x): ESP IDF Compatibility (added back CMakeLists.txt) ([#32](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/32))
- [@tueddy](https://github.com/tueddy): Compile with Arduino 3 (ESP-IDF 5.1) ([#13](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/13))
- [@vortigont](https://github.com/vortigont): Set real "Last-Modified" header based on file's LastWrite time ([#5](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/5))
- [@vortigont](https://github.com/vortigont): Some websocket code cleanup ([#29](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/29))
- [@vortigont](https://github.com/vortigont): Refactor code - replace DYI structs with STL objects  ([#39](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/39))

## Dependencies:

- **ESP32**: `mathieucarbou/Async TCP @ ^3.1.4` (Arduino IDE: [https://github.com/mathieucarbou/AsyncTCP#v3.1.4](https://github.com/mathieucarbou/AsyncTCP/releases/tag/v3.1.4))
- **ESP8266**: `esphome/ESPAsyncTCP-esphome @ 2.0.0` (Arduino IDE: [https://github.com/mathieucarbou/esphome-ESPAsyncTCP#v2.0.0](https://github.com/mathieucarbou/esphome-ESPAsyncTCP/releases/tag/v2.0.0))
- **RP2040**: `khoih-prog/AsyncTCP_RP2040W @ 1.2.0` (Arduino IDE: [https://github.com/khoih-prog/AsyncTCP_RP2040W#v1.2.0](https://github.com/khoih-prog/AsyncTCP_RP2040W/releases/tag/v1.2.0))

## Documentation

Usage and API stays the same as the original library.
Please look at the original libraries for more examples and documentation.

- [https://github.com/me-no-dev/ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) (original library)
- [https://github.com/yubox-node-org/ESPAsyncWebServer](https://github.com/yubox-node-org/ESPAsyncWebServer) (fork of the original library)

## `AsyncWebSocketMessageBuffer` and `makeBuffer()`

The fork from `yubox-node-org` introduces some breaking API changes compared to the original library, especially regarding the use of `std::shared_ptr<std::vector<uint8_t>>` for WebSocket.

This fork is compatible with the original library from `me-no-dev` regarding WebSocket, and wraps the optimizations done by `yubox-node-org` in the `AsyncWebSocketMessageBuffer` class.
So you have the choice of which API to use.

Here are examples for serializing a Json document in a websocket message buffer:

```cpp
void send(JsonDocument& doc) {
  const size_t len = measureJson(doc);

  // original API from me-no-dev
  AsyncWebSocketMessageBuffer* buffer = _ws->makeBuffer(len);
  assert(buffer); // up to you to keep or remove this
  serializeJson(doc, buffer->get(), len);
  _ws->textAll(buffer);
}
```

```cpp
void send(JsonDocument& doc) {
  const size_t len = measureJson(doc);

  // this fork (originally from yubox-node-org), uses another API with shared pointer
  auto buffer = std::make_shared<std::vector<uint8_t>>(len);
  assert(buffer); // up to you to keep or remove this
  serializeJson(doc, buffer->data(), len);
  _ws->textAll(std::move(buffer));
}
```

I recommend to use the official API `AsyncWebSocketMessageBuffer` to retain further compatibility.

## Important recommendations

Most of the crashes are caused by improper configuration of the library for the project.
Here are some recommendations to avoid them.

1. Set the running core to be on the same core of your application (usually core 1) `-D CONFIG_ASYNC_TCP_RUNNING_CORE=1`
2. Set the stack size appropriately with `-D CONFIG_ASYNC_TCP_STACK_SIZE=16384`.
   The default value of `16384` might be too much for your project.
   You can look at the [MycilaTaskMonitor](https://oss.carbou.me/MycilaTaskMonitor) project to monitor the stack usage.
3. You can change **if you know what you are doing** the task priority with `-D CONFIG_ASYNC_TCP_PRIORITY=10`.
   Default is `10`.
4. You can increase the queue size with `-D CONFIG_ASYNC_TCP_QUEUE_SIZE=128`.
   Default is `64`.
5. You can decrease the maximum ack time `-D CONFIG_ASYNC_TCP_MAX_ACK_TIME=3000`.
   Default is `5000`.

I personally use the following configuration in my projects because my WS messages can be big (up to 4k).
If you have smaller messages, you can increase `WS_MAX_QUEUED_MESSAGES` to 128.

```c++
  -D CONFIG_ASYNC_TCP_MAX_ACK_TIME=3000
  -D CONFIG_ASYNC_TCP_PRIORITY=10
  -D CONFIG_ASYNC_TCP_QUEUE_SIZE=128
  -D CONFIG_ASYNC_TCP_RUNNING_CORE=1
  -D CONFIG_ASYNC_TCP_STACK_SIZE=4096
  -D WS_MAX_QUEUED_MESSAGES=64
```
