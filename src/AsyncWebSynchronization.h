#ifndef ASYNCWEBSYNCHRONIZATION_H_
#define ASYNCWEBSYNCHRONIZATION_H_

// Synchronisation is only available on ESP32, as the ESP8266 isn't using FreeRTOS by default
#ifdef ESP32
#define WS_USE_SYNC_LOCK

#include <ESPAsyncWebServer.h>

class AsyncWebLock
{
private:
  SemaphoreHandle_t _lock;
  mutable void *_lockedBy;

public:
  AsyncWebLock() {
    _lock = xSemaphoreCreateBinary();
    _lockedBy = NULL;
    xSemaphoreGive(_lock);
  }

  ~AsyncWebLock() {
    vSemaphoreDelete(_lock);
  }

  bool lock() const {
    extern void *pxCurrentTCB;
    if (_lockedBy != pxCurrentTCB) {
      xSemaphoreTake(_lock, portMAX_DELAY);
      _lockedBy = pxCurrentTCB;
      return true;
    }
    return false;
  }

  void unlock() const {
    _lockedBy = NULL;
    xSemaphoreGive(_lock);
  }
};

class AsyncWebLockGuard
{
private:
  const AsyncWebLock *_lock;

public:
  AsyncWebLockGuard(const AsyncWebLock &l) {
    if (l.lock()) {
      _lock = &l;
    } else {
      _lock = NULL;
    }
  }

  ~AsyncWebLockGuard() {
    if (_lock) {
      _lock->unlock();
    }
  }
};
#endif

#endif // ASYNCWEBSYNCHRONIZATION_H_